import { createClient, SupabaseClient } from '@supabase/supabase-js';

const supabaseUrl = import.meta.env.VITE_SUPABASE_URL as string | undefined;
const supabaseAnonKey = import.meta.env.VITE_SUPABASE_ANON_KEY as string | undefined;

export const supabaseEnabled = Boolean(supabaseUrl && supabaseAnonKey);

export const supabase: SupabaseClient | null = supabaseEnabled
  ? createClient(supabaseUrl!, supabaseAnonKey!)
  : null;

export interface Preset {
  id: string;
  name: string;
  config: ScrcpyConfig;
  command: string;
  created_at: string;
  updated_at: string;
}

export interface DeviceEntry {
  id: string;
  serial: string;
  tcpip: string;
  enabled: boolean;
  useCommonSettings: boolean;
  overrides: Partial<ScrcpyConfig>;
}

export function createDevice(serial = '', tcpip = ''): DeviceEntry {
  return {
    id: crypto.randomUUID(),
    serial,
    tcpip,
    enabled: true,
    useCommonSettings: true,
    overrides: {},
  };
}

export interface ScrcpyConfig {
  noVideo: boolean;
  maxSize: number | null;
  bitrate: number | null;
  fps: number | null;
  codec: string;
  rotation: number;
  noDisplay: boolean;
  fullscreen: boolean;
  alwaysOnTop: boolean;
  stayAwake: boolean;
  turnScreenOff: boolean;
  disableScreensaver: boolean;
  noAudio: boolean;
  audioCodec: string;
  audioBitrate: number | null;
  recordFile: string;
  recordFormat: string;
  noControl: boolean;
  noKeyboard: boolean;
  noMouse: boolean;
  emulateKeyboard: boolean;
  otg: boolean;
  displayId: number | null;
  newDisplay: boolean;
  newDisplayWidth: number | null;
  newDisplayHeight: number | null;
  newDisplayDpi: number | null;
  serial: string;
  tcpip: string;
  multiDevice: boolean;
  devices: DeviceEntry[];
  forceAdbForward: boolean;
  powerOn: boolean;
  preferText: boolean;
  rawKeyEvents: boolean;
  verbosity: string;
  extraArgs: string;
}

export const defaultConfig: ScrcpyConfig = {
  noVideo: false,
  maxSize: null,
  bitrate: null,
  fps: null,
  codec: 'auto',
  rotation: 0,
  noDisplay: false,
  fullscreen: false,
  alwaysOnTop: false,
  stayAwake: false,
  turnScreenOff: false,
  disableScreensaver: false,
  noAudio: false,
  audioCodec: 'auto',
  audioBitrate: null,
  recordFile: '',
  recordFormat: 'mp4',
  noControl: false,
  noKeyboard: false,
  noMouse: false,
  emulateKeyboard: false,
  otg: false,
  displayId: null,
  newDisplay: false,
  newDisplayWidth: 1920,
  newDisplayHeight: 1080,
  newDisplayDpi: null,
  serial: '',
  tcpip: '',
  multiDevice: true,
  devices: [],
  forceAdbForward: false,
  powerOn: false,
  preferText: false,
  rawKeyEvents: false,
  verbosity: 'info',
  extraArgs: '',
};
