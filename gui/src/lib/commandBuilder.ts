import { ScrcpyConfig, DeviceEntry } from '@/lib/supabase';

export function buildCommand(config: ScrcpyConfig): string {
  return buildCommands(config).join('\n');
}

export function buildCommands(config: ScrcpyConfig): string[] {
  if (config.devices.length > 0) {
    const enabled = config.devices.filter((d) => d.enabled && (d.serial.trim() || d.tcpip.trim()));
    if (enabled.length > 0) {
      return enabled.map((d) =>
        buildSingleCommand(d.useCommonSettings ? config : { ...config, ...d.overrides }, d),
      );
    }
  }
  return [buildSingleCommand(config, null)];
}

function buildSingleCommand(config: ScrcpyConfig, device: DeviceEntry | null): string {
  const parts: string[] = ['scrcpy'];

  // Serial / TCP/IP — from device entry or global config
  const serial = device?.serial.trim() || config.serial.trim();
  const tcpip = device?.tcpip.trim() || config.tcpip.trim();
  if (serial) {
    parts.push('-s', serial);
  }
  if (tcpip) {
    parts.push('--tcpip=' + tcpip);
  }

  // Video
  if (config.noVideo) {
    parts.push('--no-video');
  }
  if (config.maxSize !== null && config.maxSize > 0) {
    parts.push('-m', String(config.maxSize));
  }
  if (config.bitrate !== null && config.bitrate > 0) {
    parts.push('-b', String(config.bitrate));
  }
  if (config.fps !== null && config.fps > 0) {
    parts.push('--max-fps', String(config.fps));
  }
  if (config.codec && config.codec !== 'auto') {
    parts.push('--video-codec', config.codec);
  }
  if (config.rotation !== 0) {
    parts.push('--rotation', String(config.rotation));
  }
  if (config.noDisplay) {
    parts.push('--no-display');
  }
  if (config.fullscreen) {
    parts.push('--fullscreen');
  }
  if (config.alwaysOnTop) {
    parts.push('--always-on-top');
  }
  if (config.stayAwake) {
    parts.push('--stay-awake');
  }
  if (config.turnScreenOff) {
    parts.push('--turn-screen-off');
  }
  if (config.disableScreensaver) {
    parts.push('--disable-screensaver');
  }

  // Audio
  if (config.noAudio) {
    parts.push('--no-audio');
  }
  if (config.audioCodec && config.audioCodec !== 'auto') {
    parts.push('--audio-codec', config.audioCodec);
  }
  if (config.audioBitrate !== null && config.audioBitrate > 0) {
    parts.push('--audio-bit-rate', String(config.audioBitrate));
  }

  // Recording
  if (config.recordFile) {
    parts.push('-r', config.recordFile);
  }

  // Control
  if (config.noControl) {
    parts.push('--no-control');
  }
  if (config.noKeyboard) {
    parts.push('--no-keyboard');
  }
  if (config.noMouse) {
    parts.push('--no-mouse');
  }
  if (config.emulateKeyboard) {
    parts.push('--keyboard=emu');
  }
  if (config.otg) {
    parts.push('--otg');
  }

  // Display
  if (config.displayId !== null && config.displayId >= 0) {
    parts.push('--display', String(config.displayId));
  }
  if (config.newDisplay) {
    const w = config.newDisplayWidth || 1920;
    const h = config.newDisplayHeight || 1080;
    let arg = `--new-display=${w}x${h}`;
    if (config.newDisplayDpi !== null && config.newDisplayDpi > 0) {
      arg += '/' + String(config.newDisplayDpi);
    }
    parts.push(arg);
  }

  // Advanced
  if (config.forceAdbForward) {
    parts.push('--force-adb-forward');
  }
  if (config.powerOn) {
    parts.push('--power-on');
  }
  if (config.preferText) {
    parts.push('--prefer-text');
  }
  if (config.rawKeyEvents) {
    parts.push('--raw-key-events');
  }
  if (config.verbosity && config.verbosity !== 'info') {
    parts.push('-V', config.verbosity);
  }

  // Extra args
  if (config.extraArgs.trim()) {
    parts.push(config.extraArgs.trim());
  }

  return parts.join(' ');
}
