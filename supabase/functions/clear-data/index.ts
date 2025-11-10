import { createClient } from "@supabase/supabase-js"

const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Headers': 'authorization, x-client-info, apikey, content-type',
}

Deno.serve(async (_req) => {
  if (_req.method === 'OPTIONS') {
    return new Response('ok', { headers: corsHeaders })
  }

  try {
    const supabaseUrl = Deno.env.get('SUPABASE_URL');
    const supabaseServiceKey = Deno.env.get('SUPABASE_SERVICE_ROLE_KEY');

    if (!supabaseUrl || !supabaseServiceKey) {
      throw new Error('Missing Supabase environment variables.');
    }

    const supabaseAdmin = createClient(supabaseUrl, supabaseServiceKey);

    // Delete all records from face_descriptors first
    const { error: descriptorError } = await supabaseAdmin
      .from('face_descriptors')
      .delete()
      .not('id', 'is', null); // Use a universally compatible filter

    if (descriptorError) throw descriptorError;

    // Delete all records from profiles
    const { error: profileError } = await supabaseAdmin
      .from('profiles')
      .delete()
      .not('id', 'is', null);

    if (profileError) throw profileError;

    return new Response(JSON.stringify({ message: 'All user data has been cleared successfully.' }), {
      headers: { ...corsHeaders, 'Content-Type': 'application/json' },
    });

  } catch (error) {
    console.error('Error in clear-data function:', error);
    const errorMessage = error instanceof Error ? error.message : JSON.stringify(error);
    return new Response(JSON.stringify({ error: `Function failed: ${errorMessage}` }), {
      status: 500,
      headers: { ...corsHeaders, 'Content-Type': 'application/json' },
    });
  }
})
