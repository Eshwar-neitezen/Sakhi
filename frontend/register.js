const video = document.getElementById('video');
const userNameInput = document.getElementById('userNameInput');
const registerButton = document.getElementById('registerButton');
const statusMessage = document.getElementById('statusMessage');

// --- PASTE YOUR SUPABASE KEYS HERE ---
const SUPABASE_URL = 'add_your_supabase_url';
const SUPABASE_ANON_KEY = 'SUPABASE_ANON_KEY';
const { createClient } = supabase;
const supabaseClient = createClient(SUPABASE_URL, SUPABASE_ANON_KEY);

async function setup() {
    statusMessage.innerText = 'Loading AI models...';
    try {
        await Promise.all([
            faceapi.nets.tinyFaceDetector.loadFromUri('models'),
            faceapi.nets.faceLandmark68Net.loadFromUri('models'),
            faceapi.nets.faceRecognitionNet.loadFromUri('models')
        ]);

        const stream = await navigator.mediaDevices.getUserMedia({ video: true });
        video.srcObject = stream;
        
        statusMessage.innerText = 'Camera is ready. Enter your name and click register.';
        registerButton.innerText = 'Capture and Register';
        registerButton.disabled = false;
    } catch (err) {
        console.error("Error during setup:", err);
        statusMessage.innerText = "Error: Could not start camera or load models.";
    }
}

registerButton.addEventListener('click', async () => {
    const name = userNameInput.value;
    if (!name) {
        alert('Please enter a name.');
        return;
    }

    statusMessage.innerText = 'Detecting face... Please hold still.';
    registerButton.disabled = true;

    const detection = await faceapi
        .detectSingleFace(video, new faceapi.TinyFaceDetectorOptions())
        .withFaceLandmarks()
        .withFaceDescriptor();

    // CRITICAL CHECK: Ensure a valid descriptor was created.
    if (!detection || !detection.descriptor || detection.descriptor.length !== 128) {
        statusMessage.innerText = 'Could not generate a valid face fingerprint. Please try again with better lighting and a clearer view of your face.';
        registerButton.disabled = false;
        return;
    }

    statusMessage.innerText = 'Valid face detected! Saving to database...';
    
    try {
        const { data: profileData, error: profileError } = await supabaseClient
            .from('profiles')
            .insert({ name: name })
            .select('id')
            .single();

        if (profileError) throw profileError;
        
        const { error: descriptorError } = await supabaseClient
            .from('face_descriptors')
            .insert({ 
              user_id: profileData.id, 
              embedding: Array.from(detection.descriptor)
            });
        
        if (descriptorError) throw descriptorError;
        
        statusMessage.innerText = `Welcome, ${name}! You are now registered.`;
        alert(`Welcome, ${name}! You can now use the login page.`);
        registerButton.disabled = false;

    } catch (error) {
        console.error('Registration failed:', error);
        statusMessage.innerText = `Registration failed: ${error.message}`;
        registerButton.disabled = false;
    }
});

setup();
