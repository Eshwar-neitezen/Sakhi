// --- Get HTML Elements ---
const homeScreen = document.getElementById('home-screen');
const alzheimerModeScreen = document.getElementById('alzheimer-mode');
const loadingOverlay = document.getElementById('loading-overlay');
const loadingMessage = document.getElementById('loading-message');
const transcriptOutput = document.getElementById('transcript-output');

// Alzheimer Mode Elements
const registerBtn = document.getElementById('register-face-btn');
const recognizeBtn = document.getElementById('recognize-face-btn');
const userNameInput = document.getElementById('userNameInput');
const faceStatus = document.getElementById('face-status');
const faceRecContainer = document.getElementById('face-recognition-container');
document.querySelectorAll('.home-btn').forEach(btn => btn.addEventListener('click', () => showScreen('home')));

// --- SUPABASE & FACE-API CONFIG ---
const { createClient } = supabase;
const supabaseClient = createClient(SUPABASE_URL, SUPABASE_ANON_KEY);

let faceMatcher = null;

// --- INITIAL SETUP ---
async function start() {
    loadingMessage.innerText = 'Loading AI models...';
    await Promise.all([
        faceapi.nets.tinyFaceDetector.loadFromUri('models'),
        faceapi.nets.faceLandmark68Net.loadFromUri('models'),
        faceapi.nets.faceRecognitionNet.loadFromUri('models')
    ]);
    loadingOverlay.style.display = 'none';
    startContinuousRecognition(); // Start listening for the wake word
}

// --- SCREEN MANAGEMENT ---
function showScreen(screenName) {
    // Hide all screens
    document.querySelectorAll('.screen').forEach(s => s.classList.add('hidden'));
    // Show the requested screen
    document.getElementById(screenName).classList.remove('hidden');
}

// --- VOICE COMMAND ROUTER ---
const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
const recognition = new SpeechRecognition();
recognition.continuous = true;
recognition.interimResults = true;

recognition.onresult = (event) => {
    let finalTranscript = '';
    for (let i = 0; i < event.results.length; i++) {
        finalTranscript += event.results[i][0].transcript;
    }
    
    transcriptOutput.innerText = `I heard: "${finalTranscript}"`;

    if (finalTranscript.toLowerCase().includes("sakhi")) {
        console.log("Wake word detected!");
        recognition.stop(); // Stop listening to process the command
        processCommand(finalTranscript.toLowerCase());
    }
};

recognition.onend = () => {
    // Automatically restart listening after a command is processed
    setTimeout(() => {
        if (!recognition.running) recognition.start();
    }, 500);
};

function startContinuousRecognition() {
    try {
        recognition.start();
        recognition.running = true;
        console.log("Started continuous listening.");
    } catch(e) {
        console.error("Recognition already started.", e);
    }
}

function processCommand(command) {
    if (command.includes("alzheimer mode") || command.includes("alzhiemer mode")) {
        speak("Switching to Alzheimer mode.");
        showScreen('alzheimer-mode');
    } else if (command.includes("blind mode")) {
        speak("Blind mode is not yet implemented.");
        // showScreen('blind-mode'); // Future implementation
    } else if (command.includes("go home")) {
        speak("Returning to the home screen.");
        showScreen('home');
    } else {
        // If no keyword is found, treat it as a chat command
        handleChatCommand(command.replace('sakhi', '').trim());
    }
}

async function handleChatCommand(command) {
    try {
        const { data, error } = await supabaseClient.functions.invoke('chat-handler', {
            body: { message: command }
        });
        if (error) throw error;
        speak(data.reply);
    } catch (error) {
        console.error("Error communicating with chat function:", error);
        speak("I'm having a little trouble thinking right now.");
    }
}

function speak(text) {
    const utterance = new SpeechSynthesisUtterance(text);
    speechSynthesis.speak(utterance);
}

// --- FACE RECOGNITION LOGIC (for Alzheimer Mode) ---
let videoEl = null;
let recognitionInterval = null;

async function startFaceRecognition(mode) { // mode can be 'register' or 'recognize'
    if (videoEl) {
        stopFaceRecognition(); // Stop any existing streams
    }

    faceStatus.innerText = 'Starting camera...';
    videoEl = document.createElement('video');
    videoEl.width = 640;
    videoEl.height = 480;
    videoEl.autoplay = true;
    videoEl.muted = true;
    faceRecContainer.innerHTML = ''; // Clear previous video
    faceRecContainer.append(videoEl);

    const stream = await navigator.mediaDevices.getUserMedia({ video: true });
    videoEl.srcObject = stream;
    
    videoEl.addEventListener('play', () => {
        if (mode === 'recognize') {
            runRecognitionLoop();
        } else if (mode === 'register') {
            faceStatus.innerText = 'Camera ready. Position your face and click Register.';
        }
    });
}

function stopFaceRecognition() {
    if (recognitionInterval) clearInterval(recognitionInterval);
    if (videoEl && videoEl.srcObject) {
        videoEl.srcObject.getTracks().forEach(track => track.stop());
    }
    faceRecContainer.innerHTML = '';
    videoEl = null;
}

// Attach functions to buttons
registerBtn.addEventListener('click', () => {
    if (!videoEl) {
        startFaceRecognition('register');
    } else {
        registerNewUser();
    }
});
recognizeBtn.addEventListener('click', () => startFaceRecognition('recognize'));
document.querySelector('#alzheimer-mode .home-btn').addEventListener('click', stopFaceRecognition);


async function registerNewUser() {
    // This is the logic from your old register.js
    const name = userNameInput.value;
    if (!name) return alert('Please enter a name.');
    
    faceStatus.innerText = 'Detecting face...';
    const detection = await faceapi.detectSingleFace(videoEl).withFaceLandmarks().withFaceDescriptor();
    if (!detection || detection.descriptor.length !== 128) {
        return faceStatus.innerText = 'Could not detect a valid face. Please try again.';
    }

    faceStatus.innerText = 'Registering...';
    const { data, error } = await supabaseClient.functions.invoke('register-face', {
        body: { name, descriptor: Array.from(detection.descriptor) }
    });

    if (error) {
        faceStatus.innerText = `Registration failed: ${error.message}`;
    } else {
        faceStatus.innerText = `Welcome, ${name}! You are now registered.`;
        stopFaceRecognition(); // Turn off camera after success
    }
}

async function runRecognitionLoop() {
    // This is the logic from your old login script
    faceStatus.innerText = 'Loading registered faces...';
    const labeledDescriptors = await loadRegisteredFaces();
    if (labeledDescriptors.length === 0) {
        return faceStatus.innerText = 'No faces are registered.';
    }
    faceMatcher = new faceapi.FaceMatcher(labeledDescriptors, 0.6);
    
    const canvas = faceapi.createCanvasFromMedia(videoEl);
    faceRecContainer.append(canvas);
    const displaySize = { width: videoEl.width, height: videoEl.height };
    faceapi.matchDimensions(canvas, displaySize);

    recognitionInterval = setInterval(async () => {
        const detection = await faceapi.detectSingleFace(videoEl).withFaceLandmarks().withFaceDescriptor();
        canvas.getContext('2d').clearRect(0, 0, canvas.width, canvas.height);

        if (detection) {
            const bestMatch = faceMatcher.findBestMatch(detection.descriptor);
            const box = faceapi.resizeResults(detection, displaySize).detection.box;
            const drawBox = new faceapi.draw.DrawBox(box, { label: bestMatch.toString() });
            drawBox.draw(canvas);
            
            if (bestMatch.label !== 'unknown') {
                faceStatus.innerText = `Hello, ${bestMatch.label}!`;
            }
        }
    }, 200);
}

async function loadRegisteredFaces() {
    // This function is the same as before
    const { data, error } = await supabaseClient.from('face_descriptors').select(`embedding, profiles(name)`);
    if (error) return [];
    const validData = data.filter(d => d.embedding && d.embedding.length === 128 && d.profiles);
    return validData.map(fd => new faceapi.LabeledFaceDescriptors(fd.profiles.name, [new Float32Array(fd.embedding)]));
}

// Start the whole application
start();
