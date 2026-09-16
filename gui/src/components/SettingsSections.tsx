import {
  Video, Volume2, Disc, MousePointer, Monitor, Settings2,
  ChevronDown, ChevronRight,
} from 'lucide-react';
import { useState } from 'react';
import { ScrcpyConfig } from '@/lib/supabase';

export const videoCodecs = ['auto', 'h264', 'h265', 'av1'];
export const audioCodecs = ['auto', 'aac', 'opus', 'raw'];
export const verbosityLevels = ['quiet', 'error', 'warn', 'info', 'debug', 'trace'];
export const recordFormats = ['mp4', 'mkv', 'm4a', 'mka', 'opus', 'aac', 'wav'];

type SectionKey = 'video' | 'audio' | 'recording' | 'control' | 'display' | 'advanced';

interface SectionMeta {
  key: SectionKey;
  label: string;
  icon: typeof Video;
  description: string;
}

const sections: SectionMeta[] = [
  { key: 'video', label: 'Video', icon: Video, description: 'Resolution, bitrate, codec, display' },
  { key: 'audio', label: 'Audio', icon: Volume2, description: 'Audio forwarding and codec' },
  { key: 'recording', label: 'Recording', icon: Disc, description: 'Record screen to file' },
  { key: 'control', label: 'Control', icon: MousePointer, description: 'Keyboard, mouse, input modes' },
  { key: 'display', label: 'Display', icon: Monitor, description: 'Virtual display, multi-display' },
  { key: 'advanced', label: 'Advanced', icon: Settings2, description: 'ADB, verbosity, extra args' },
];

export function Toggle({ checked, onChange, label, hint }: {
  checked: boolean;
  onChange: (v: boolean) => void;
  label: string;
  hint?: string;
}) {
  return (
    <button
      type="button"
      onClick={() => onChange(!checked)}
      className={`flex items-center justify-between w-full p-3.5 rounded-xl border transition-all duration-200 text-left group ${
        checked
          ? 'bg-emerald-50/80 border-emerald-400 shadow-sm'
          : 'bg-white border-slate-200 hover:border-slate-300 hover:bg-slate-50/50'
      }`}
    >
      <div className="flex items-center gap-3">
        <div className={`w-10 h-6 rounded-full transition-colors duration-200 flex items-center p-0.5 ${
          checked ? 'bg-emerald-500' : 'bg-slate-300'
        }`}>
          <div className={`w-5 h-5 rounded-full bg-white shadow-sm transition-transform duration-200 ${
            checked ? 'translate-x-4' : 'translate-x-0'
          }`} />
        </div>
        <div>
          <span className="text-sm font-medium text-slate-800">{label}</span>
          {hint && <p className="text-xs text-slate-400 mt-0.5">{hint}</p>}
        </div>
      </div>
    </button>
  );
}

export function NumberField({ value, onChange, label, placeholder, suffix }: {
  value: number | null;
  onChange: (v: number | null) => void;
  label: string;
  placeholder?: string;
  suffix?: string;
}) {
  return (
    <div>
      <label className="block text-xs font-semibold text-slate-500 uppercase tracking-wide mb-1.5">{label}</label>
      <div className="relative">
        <input
          type="number"
          value={value ?? ''}
          onChange={(e) => onChange(e.target.value === '' ? null : Number(e.target.value))}
          placeholder={placeholder}
          className="w-full px-3.5 py-2.5 rounded-xl border border-slate-200 bg-white text-sm text-slate-800 placeholder:text-slate-300 focus:outline-none focus:ring-2 focus:ring-emerald-400/40 focus:border-emerald-400 transition-all"
        />
        {suffix && (
          <span className="absolute right-3.5 top-1/2 -translate-y-1/2 text-xs text-slate-400 font-medium pointer-events-none">{suffix}</span>
        )}
      </div>
    </div>
  );
}

export function SelectField({ value, onChange, label, options }: {
  value: string;
  onChange: (v: string) => void;
  label: string;
  options: string[];
}) {
  return (
    <div>
      <label className="block text-xs font-semibold text-slate-500 uppercase tracking-wide mb-1.5">{label}</label>
      <select
        value={value}
        onChange={(e) => onChange(e.target.value)}
        className="w-full px-3.5 py-2.5 rounded-xl border border-slate-200 bg-white text-sm text-slate-800 focus:outline-none focus:ring-2 focus:ring-emerald-400/40 focus:border-emerald-400 transition-all cursor-pointer"
      >
        {options.map((opt) => (
          <option key={opt} value={opt}>{opt === 'auto' ? 'Auto (default)' : opt}</option>
        ))}
      </select>
    </div>
  );
}

export function TextField({ value, onChange, label, placeholder }: {
  value: string;
  onChange: (v: string) => void;
  label: string;
  placeholder?: string;
}) {
  return (
    <div>
      <label className="block text-xs font-semibold text-slate-500 uppercase tracking-wide mb-1.5">{label}</label>
      <input
        type="text"
        value={value}
        onChange={(e) => onChange(e.target.value)}
        placeholder={placeholder}
        className="w-full px-3.5 py-2.5 rounded-xl border border-slate-200 bg-white text-sm text-slate-800 placeholder:text-slate-300 focus:outline-none focus:ring-2 focus:ring-emerald-400/40 focus:border-emerald-400 transition-all"
      />
    </div>
  );
}

function SectionCard({ section, isOpen, onToggle, children }: {
  section: SectionMeta;
  isOpen: boolean;
  onToggle: () => void;
  children: React.ReactNode;
}) {
  const Icon = section.icon;
  return (
    <div className="bg-white rounded-2xl border border-slate-200 overflow-hidden transition-all duration-200">
      <button
        type="button"
        onClick={onToggle}
        className="flex items-center justify-between w-full p-4 hover:bg-slate-50/50 transition-colors"
      >
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-xl bg-slate-100 flex items-center justify-center text-slate-600">
            <Icon className="w-5 h-5" />
          </div>
          <div className="text-left">
            <h3 className="text-sm font-semibold text-slate-800">{section.label}</h3>
            <p className="text-xs text-slate-400">{section.description}</p>
          </div>
        </div>
        {isOpen ? <ChevronDown className="w-5 h-5 text-slate-400" /> : <ChevronRight className="w-5 h-5 text-slate-400" />}
      </button>
      <div className={`grid transition-all duration-200 ease-in-out ${
        isOpen ? 'grid-rows-[1fr] opacity-100' : 'grid-rows-[0fr] opacity-0'
      }`}>
        <div className="overflow-hidden">
          <div className="p-4 pt-0 space-y-4">{children}</div>
        </div>
      </div>
    </div>
  );
}

export function SettingsSections({ config, update, defaultOpen }: {
  config: ScrcpyConfig;
  update: <K extends keyof ScrcpyConfig>(key: K, value: ScrcpyConfig[K]) => void;
  defaultOpen?: SectionKey[];
}) {
  const [openSections, setOpenSections] = useState<Set<SectionKey>>(
    new Set(defaultOpen ?? ['video'])
  );

  const toggleSection = (key: SectionKey) => {
    setOpenSections((prev) => {
      const next = new Set(prev);
      if (next.has(key)) next.delete(key);
      else next.add(key);
      return next;
    });
  };

  return (
    <div className="space-y-3">
      {/* Video */}
      <SectionCard section={sections[0]} isOpen={openSections.has('video')} onToggle={() => toggleSection('video')}>
        <div className="grid grid-cols-2 gap-3">
          <NumberField label="Max Size" value={config.maxSize} onChange={(v) => update('maxSize', v)} placeholder="e.g. 1920" suffix="px" />
          <NumberField label="Bitrate" value={config.bitrate} onChange={(v) => update('bitrate', v)} placeholder="e.g. 8000000" suffix="bps" />
          <NumberField label="Max FPS" value={config.fps} onChange={(v) => update('fps', v)} placeholder="e.g. 60" suffix="fps" />
          <SelectField label="Video Codec" value={config.codec} onChange={(v) => update('codec', v)} options={videoCodecs} />
        </div>
        <div>
          <label className="block text-xs font-semibold text-slate-500 uppercase tracking-wide mb-1.5">Rotation</label>
          <div className="flex gap-2">
            {[0, 90, 180, 270].map((r) => (
              <button
                key={r}
                onClick={() => update('rotation', r)}
                className={`flex-1 py-2.5 rounded-xl text-sm font-medium transition-all ${
                  config.rotation === r
                    ? 'bg-emerald-500 text-white shadow-sm shadow-emerald-500/20'
                    : 'bg-slate-100 text-slate-600 hover:bg-slate-200'
                }`}
              >
                {r}°
              </button>
            ))}
          </div>
        </div>
        <div className="space-y-2">
          <Toggle checked={config.noVideo} onChange={(v) => update('noVideo', v)} label="No Video" hint="Disable video forwarding" />
          <Toggle checked={config.noDisplay} onChange={(v) => update('noDisplay', v)} label="No Display" hint="Don't show the mirror window" />
          <Toggle checked={config.fullscreen} onChange={(v) => update('fullscreen', v)} label="Fullscreen" hint="Launch in fullscreen mode" />
          <Toggle checked={config.alwaysOnTop} onChange={(v) => update('alwaysOnTop', v)} label="Always On Top" hint="Keep window on top of others" />
          <Toggle checked={config.stayAwake} onChange={(v) => update('stayAwake', v)} label="Stay Awake" hint="Keep device awake while mirroring" />
          <Toggle checked={config.turnScreenOff} onChange={(v) => update('turnScreenOff', v)} label="Turn Screen Off" hint="Turn off physical screen" />
          <Toggle checked={config.disableScreensaver} onChange={(v) => update('disableScreensaver', v)} label="Disable Screensaver" hint="Prevent screensaver on computer" />
        </div>
      </SectionCard>

      {/* Audio */}
      <SectionCard section={sections[1]} isOpen={openSections.has('audio')} onToggle={() => toggleSection('audio')}>
        <div className="grid grid-cols-2 gap-3">
          <SelectField label="Audio Codec" value={config.audioCodec} onChange={(v) => update('audioCodec', v)} options={audioCodecs} />
          <NumberField label="Audio Bitrate" value={config.audioBitrate} onChange={(v) => update('audioBitrate', v)} placeholder="e.g. 128000" suffix="bps" />
        </div>
        <Toggle checked={config.noAudio} onChange={(v) => update('noAudio', v)} label="No Audio" hint="Disable audio forwarding" />
      </SectionCard>

      {/* Recording */}
      <SectionCard section={sections[2]} isOpen={openSections.has('recording')} onToggle={() => toggleSection('recording')}>
        <div className="grid grid-cols-2 gap-3">
          <TextField label="Record File" value={config.recordFile} onChange={(v) => update('recordFile', v)} placeholder="recording.mp4" />
          <SelectField label="Format" value={config.recordFormat} onChange={(v) => update('recordFormat', v)} options={recordFormats} />
        </div>
      </SectionCard>

      {/* Control */}
      <SectionCard section={sections[3]} isOpen={openSections.has('control')} onToggle={() => toggleSection('control')}>
        <div className="space-y-2">
          <Toggle checked={config.noControl} onChange={(v) => update('noControl', v)} label="No Control" hint="Mirror only, no input control" />
          <Toggle checked={config.noKeyboard} onChange={(v) => update('noKeyboard', v)} label="No Keyboard" hint="Disable keyboard injection" />
          <Toggle checked={config.noMouse} onChange={(v) => update('noMouse', v)} label="No Mouse" hint="Disable mouse injection" />
          <Toggle checked={config.emulateKeyboard} onChange={(v) => update('emulateKeyboard', v)} label="Emulate Keyboard" hint="Simulate a physical keyboard" />
          <Toggle checked={config.otg} onChange={(v) => update('otg', v)} label="OTG Mode" hint="Control via USB without USB debugging" />
        </div>
      </SectionCard>

      {/* Display */}
      <SectionCard section={sections[4]} isOpen={openSections.has('display')} onToggle={() => toggleSection('display')}>
        <NumberField label="Display ID" value={config.displayId} onChange={(v) => update('displayId', v)} placeholder="e.g. 0" />
        <div className="border-t border-slate-100 pt-3 space-y-2">
          <Toggle checked={config.newDisplay} onChange={(v) => update('newDisplay', v)} label="New Virtual Display" hint="Create a separate display" />
          {config.newDisplay && (
            <div className="grid grid-cols-3 gap-2 pl-3 border-l-2 border-emerald-200">
              <NumberField label="Width" value={config.newDisplayWidth} onChange={(v) => update('newDisplayWidth', v)} placeholder="1920" />
              <NumberField label="Height" value={config.newDisplayHeight} onChange={(v) => update('newDisplayHeight', v)} placeholder="1080" />
              <NumberField label="DPI" value={config.newDisplayDpi} onChange={(v) => update('newDisplayDpi', v)} placeholder="240" />
            </div>
          )}
        </div>
      </SectionCard>

      {/* Advanced */}
      <SectionCard section={sections[5]} isOpen={openSections.has('advanced')} onToggle={() => toggleSection('advanced')}>
        <div className="space-y-2">
          <Toggle checked={config.forceAdbForward} onChange={(v) => update('forceAdbForward', v)} label="Force ADB Forward" hint="Use adb forward instead of push" />
          <Toggle checked={config.powerOn} onChange={(v) => update('powerOn', v)} label="Power On" hint="Power on the device on connect" />
          <Toggle checked={config.preferText} onChange={(v) => update('preferText', v)} label="Prefer Text" hint="Inject text instead of key events" />
          <Toggle checked={config.rawKeyEvents} onChange={(v) => update('rawKeyEvents', v)} label="Raw Key Events" hint="Inject raw key events" />
        </div>
        <SelectField label="Verbosity" value={config.verbosity} onChange={(v) => update('verbosity', v)} options={verbosityLevels} />
        <div>
          <label className="block text-xs font-semibold text-slate-500 uppercase tracking-wide mb-1.5">Extra Arguments</label>
          <input
            type="text"
            value={config.extraArgs}
            onChange={(e) => update('extraArgs', e.target.value)}
            placeholder="--any-extra-flag value"
            className="w-full px-3.5 py-2.5 rounded-xl border border-slate-200 bg-white text-sm font-mono text-slate-800 placeholder:text-slate-300 focus:outline-none focus:ring-2 focus:ring-emerald-400/40 focus:border-emerald-400 transition-all"
          />
        </div>
      </SectionCard>
    </div>
  );
}
