import { Preset, ScrcpyConfig, supabase, supabaseEnabled } from '@/lib/supabase';

const API = '/api';

export async function scanDevices(): Promise<{ serial: string; tcpip?: string; state?: string; model?: string }[]> {
  const res = await fetch(`${API}/scan-devices`);
  if (!res.ok) {
    const err = await res.json().catch(() => ({}));
    throw new Error(err.error || 'Scan failed');
  }
  const data = await res.json();
  return data.devices || [];
}

export async function startCommands(commands: string[]): Promise<void> {
  const res = await fetch(`${API}/start`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ commands }),
  });
  if (!res.ok) {
    const err = await res.json().catch(() => ({}));
    throw new Error(err.error || 'Failed to start');
  }
}

export async function stopSessions(serial?: string): Promise<void> {
  const res = await fetch(`${API}/stop`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(serial ? { serial } : {}),
  });
  if (!res.ok) {
    const err = await res.json().catch(() => ({}));
    throw new Error(err.error || 'Failed to stop');
  }
}

export async function fetchSessions(): Promise<{ serial: string; alive: boolean; command: string }[]> {
  const res = await fetch(`${API}/sessions`);
  if (!res.ok) return [];
  const data = await res.json();
  return data.sessions || [];
}

export async function loadPresets(): Promise<Preset[]> {
  if (supabaseEnabled && supabase) {
    const { data, error } = await supabase
      .from('scrcpy_presets')
      .select('*')
      .order('updated_at', { ascending: false });
    if (error) throw error;
    return (data || []) as Preset[];
  }

  const res = await fetch(`${API}/presets`);
  if (!res.ok) throw new Error('Failed to load presets');
  const data = await res.json();
  return (data.presets || []) as Preset[];
}

export async function savePreset(name: string, config: ScrcpyConfig, command: string): Promise<void> {
  if (supabaseEnabled && supabase) {
    const { error } = await supabase.from('scrcpy_presets').insert({ name, config, command });
    if (error) throw error;
    return;
  }

  const res = await fetch(`${API}/presets`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ name, config, command }),
  });
  if (!res.ok) {
    const err = await res.json().catch(() => ({}));
    throw new Error(err.error || 'Failed to save preset');
  }
}

export async function deletePreset(id: string): Promise<void> {
  if (supabaseEnabled && supabase) {
    const { error } = await supabase.from('scrcpy_presets').delete().eq('id', id);
    if (error) throw error;
    return;
  }

  const res = await fetch(`${API}/presets/${encodeURIComponent(id)}`, { method: 'DELETE' });
  if (!res.ok) throw new Error('Failed to delete preset');
}
