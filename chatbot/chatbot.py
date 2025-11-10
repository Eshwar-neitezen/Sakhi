import google.generativeai as genai
import os
from dotenv import load_dotenv

# Load API key from .env file
load_dotenv()
API_KEY = os.getenv("GEMINI_API_KEY")

if not API_KEY:
    print("Error: GEMINI_API_KEY not found. Please check your .env file.")
    exit()

# Configure the Gemini client
genai.configure(api_key=API_KEY)

# Initialize the Gemini model
print("Initializing Sakhi's brain (Gemini)...")
model = genai.GenerativeModel(model_name="gemini-1.5-pro-latest")
print("Chatbot is ready.")

def get_chatbot_response(user_input):
    """
    Sends the user's message to the Gemini API and returns the response.
    """
    try:
        response = model.generate_content(user_input)
        return response.text
    except Exception as e:
        print(f"An error occurred: {e}")
        return "Sorry, I'm having trouble thinking right now."


if __name__ == "__main__":
    print("This script is intended to be imported as a module.")
 