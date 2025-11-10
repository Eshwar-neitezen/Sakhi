// Define CORS headers
const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Headers': 'authorization, x-client-info, apikey, content-type',
}

// Get your secret keys from the Supabase dashboard
const GEMINI_API_KEY = Deno.env.get('GEMINI_API_KEY');
const IFTTT_WEBHOOK_KEY = Deno.env.get('IFTTT_WEBHOOK_KEY');
const GEMINI_API_URL = `https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent?key=${GEMINI_API_KEY}`;

Deno.serve(async (req) => {
  // Handle CORS preflight request
  if (req.method === 'OPTIONS') {
    return new Response('ok', { headers: corsHeaders })
  }

  try {
    const { message } = await req.json();
    const userQuery = message.toLowerCase();

    // --- Hardware & IFTTT Command Logic ---
    let lightCommand = null;
    if (userQuery.includes("turn on light")) lightCommand = "ON";
    else if (userQuery.includes("turn off light")) lightCommand = "OFF";

    if (lightCommand) {
      const iftttUrl = `https://maker.ifttt.com/trigger/toggle_light/with/key/${IFTTT_WEBHOOK_KEY}`;
      // Trigger the webhook but don't wait for the response
      fetch(iftttUrl, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ value1: lightCommand }),
      }).catch(e => console.error("IFTTT Error:", e));

      return new Response(JSON.stringify({ reply: `Okay, turning the light ${lightCommand.toLowerCase()}.` }), {
        headers: { ...corsHeaders, 'Content-Type': 'application/json' },
      });
    }
    
    // --- If no hardware command, use Gemini for a chat response ---
    const geminiResponse = await fetch(GEMINI_API_URL, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        contents: [{ parts: [{ text: `You are Sakhi, a friendly personal assistant robot. A user says: '${message}'. Respond concisely and helpfully.` }] }],
      }),
    });

    if (!geminiResponse.ok) {
        throw new Error(`Gemini API failed with status: ${geminiResponse.statusText}`);
    }

    const geminiData = await geminiResponse.json();
    
    if (!geminiData.candidates || geminiData.candidates.length === 0) {
      console.error("Gemini API response missing candidates:", geminiData);
      throw new Error("No response from AI assistant.");
    }

    const reply = geminiData.candidates[0].content.parts[0].text;

    return new Response(JSON.stringify({ reply }), {
      headers: { ...corsHeaders, 'Content-Type': 'application/json' },
    });

  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : 'An unknown error occurred.';
    return new Response(JSON.stringify({ error: errorMessage }), {
      status: 400, // Use 400 for a bad request
      headers: { ...corsHeaders, 'Content-Type': 'application/json' },
    });
  }
})
