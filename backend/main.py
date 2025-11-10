import os
import requests
from dotenv import load_dotenv
from fastapi import FastAPI, HTTPException, Body
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
from typing import List, Optional
import pymongo
import google.generativeai as genai
import json

# --- Load Environment Variables ---
load_dotenv()
MONGO_URI = os.getenv("MONGO_URI")
GEMINI_API_KEY = os.getenv("GEMINI_API_KEY")

# --- Initialize Application and Services ---
app = FastAPI()

# Configure Gemini API
genai.configure(api_key=GEMINI_API_KEY)
gemini_model = genai.GenerativeModel('gemini-pro')

# THIS IS THE CRUCIAL PART THAT FIXES THE CORS ERROR
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"], # Allows all origins, including your frontend
    allow_credentials=True,
    allow_methods=["*"], # Allows all HTTP methods (GET, POST, etc.)
    allow_headers=["*"], # Allows all headers
)
# --- Database Connection (MongoDB) ---
try:
    client = pymongo.MongoClient(MONGO_URI)
    db = client.sakhi_robot
    users_collection = db.users
    print("MongoDB connection successful.")
except Exception as e:
    print(f"Error connecting to MongoDB: {e}")

# --- Pydantic Models (Data Validation) ---
class SOSContact(BaseModel):
    name: str
    phone: str

class Reminder(BaseModel):
    time: str
    task: str

class User(BaseModel):
    face_id: str = Field(..., alias="_id") # Use face_id as the document ID
    name: str
    reminders: List[Reminder] = []
    sos_contacts: List[SOSContact] = []

class ChatMessage(BaseModel):
    face_id: str
    message: str


esp32_command_queue = {"command": "none"}

@app.get("/")
def read_root():
    return {"message": "Welcome to Sakhi Personal Assistant API"}

@app.post("/register")
async def register_user(user: User):
    """Registers a new user or updates an existing one in the database."""
    user_dict = user.dict(by_alias=True)
    try:
        # Update if exists, insert if not (upsert)
        users_collection.update_one(
            {"_id": user.face_id},
            {"$set": user_dict},
            upsert=True
        )
        return {"message": f"User {user.name} registered/updated successfully."}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/user/{face_id}", response_model=User)
async def get_user_by_face_id(face_id: str):
    """Retrieves user data from MongoDB using their face_id."""
    user_data = users_collection.find_one({"_id": face_id})
    if user_data:
        return user_data
    raise HTTPException(status_code=404, detail="User not found")

@app.post("/chat")
async def handle_chat(chat_message: ChatMessage):
    """Handles user voice commands, interacts with Gemini, and triggers actions."""
    user_query = chat_message.message.lower()
    
    # --- Command & Control Logic ---
    # Check for specific keywords to trigger actions directly
    if "turn on the light" in user_query:
        esp32_command_queue["command"] = "led_on"
        return {"reply": "Okay, turning on the light."}
    
    if "turn off the light" in user_query:
        esp32_command_queue["command"] = "led_off"
        return {"reply": "Okay, turning off the light."}
        
    if "wave your hand" in user_query:
        esp32_command_queue["command"] = "servo_wave"
        return {"reply": "Sure, waving my hand!"}
        
    if "sos" in user_query or "emergency" in user_query:
        # Fetch user to get their name for the alert
        user = users_collection.find_one({"_id": chat_message.face_id})
        user_name = user.get("name", "A user") if user else "A user"
        trigger_ifttt_webhook("sos_alert", value1=user_name, value2="Location: Home")
        return {"reply": "Emergency alert triggered. I've notified your contacts."}

    # --- If no direct command, use Gemini for a conversational reply ---
    try:
        # A simple prompt for Gemini
        prompt = f"You are Sakhi, a friendly personal assistant robot. A user says: '{chat_message.message}'. Respond in a helpful and concise manner."
        response = gemini_model.generate_content(prompt)
        return {"reply": response.text}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error with Gemini API: {str(e)}")

@app.get("/get-command")
async def get_esp32_command():
    """Endpoint for the ESP32 to poll for new commands."""
    command = esp32_command_queue["command"]
    # Reset the command after it's been fetched
    if command != "none":
        esp32_command_queue["command"] = "none"
    return {"command": command}

# To run the app: uvicorn main:app --reload --host 0.0.0.0 --port 8000