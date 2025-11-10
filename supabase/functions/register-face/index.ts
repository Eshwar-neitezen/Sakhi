import { createClient } from '@supabase/supabase-js'

const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Headers': 'authorization, x-client-info, apikey, content-type',
}

Deno.serve(async (req) => {
  if (req.method === 'OPTIONS') {
    return new Response('ok', { headers: corsHeaders })
  }

  try {
    const supabaseAdmin = createClient(
      Deno.env.get('SUPABASE_URL') ?? '',
      Deno.env.get('SUPABASE_SERVICE_ROLE_KEY') ?? ''
    );

    const { name, descriptor } = await req.json();

    // --- NEW: SERVER-SIDE VALIDATION ---
    // This check ensures we only accept valid data from the frontend.
    if (!name || !descriptor || !Array.isArray(descriptor) || descriptor.length !== 128) {
      throw new Error('Invalid request: Name is required and descriptor must be an array of 128 numbers.');
    }
    // --- END OF VALIDATION ---

    // (The rest of the function is the same as before)
    const { data: profileData, error: profileError } = await supabaseAdmin
      .from('profiles')
      .insert({ name: name })
      .select('id')
      .single();
    if (profileError) throw profileError;

    const { error: descriptorError } = await supabaseAdmin
      .from('face_descriptors')
      .insert({ user_id: profileData.id, embedding: descriptor });
    if (descriptorError) {
      await supabaseAdmin.from('profiles').delete().eq('id', profileData.id);
      throw descriptorError;
    }

    return new Response(JSON.stringify({ 
      message: 'User registered successfully!', 
      userId: profileData.id 
    }), {
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
