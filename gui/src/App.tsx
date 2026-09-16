import { useState, useEffect, useCallback, useRef } from 'react';
import {
  Smartphone, Video, Volume2, Disc, MousePointer, Monitor, Wifi,
  Settings2, Copy, Check, Save, Trash2, FolderOpen, RotateCw,
  Terminal, ChevronDown, ChevronRight,   Zap, X, Plus, Search,
  Layers, Eye, Lock, Sun, Bell, Keyboard, Gamepad2, Power,
  FileText, AlertCircle, CheckCircle2, RefreshCw, ChevronLeft,
  Sliders, Usb, Play, Square,
} from 'lucide-react';
import { supabaseEnabled } from '@/lib/supabase';
import { Preset, ScrcpyConfig, defaultConfig, createDevice } from '@/lib/supabase';
import { buildCommands } from '@/lib/commandBuilder';
import {
  SettingsSections, Toggle, TextField,
} from '@/components/SettingsSections';
import {
  scanDevices, startCommands, stopSessions, loadPresets as fetchPresets,
  savePreset, deletePreset, fetchSessions,
} from '@/lib/api';

function App() {
  const [config, setConfig] = useState<ScrcpyConfig>(defaultConfig);
  const [command, setCommand] = useState('scrcpy');
  const [commands, setCommands] = useState<string[]>(['scrcpy']);
  const [presets, setPresets] = useState<Preset[]>([]);
  const [presetName, setPresetName] = useState('');
  const [showSaveDialog, setShowSaveDialog] = useState(false);
  const [showPresets, setShowPresets] = useState(false);
  const [copied, setCopied] = useState(false);
  const [loading, setLoading] = useState(true);
  const [toast, setToast] = useState<{ msg: string; type: 'success' | 'error' } | null>(null);
  const toastTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
  const [showCommonSettings, setShowCommonSettings] = useState(false);
  const [editingDeviceId, setEditingDeviceId] = useState<string | null>(null);
  const [scanning, setScanning] = useState(false);
  const [scanError, setScanError] = useState<string | null>(null);
  const [running, setRunning] = useState(false);
  const [activeSessions, setActiveSessions] = useState(0);

  const showToast = useCallback((msg: string, type: 'success' | 'error' = 'success') => {
    setToast({ msg, type });
    if (toastTimer.current) clearTimeout(toastTimer.current);
    toastTimer.current = setTimeout(() => setToast(null), 2500);
  }, []);

  useEffect(() => {
    const cmds = buildCommands(config);
    setCommands(cmds);
    setCommand(cmds.join('\n'));
  }, [config]);

  useEffect(() => {
    loadPresets();
    refreshSessions();
    const t = setInterval(refreshSessions, 2000);
    return () => clearInterval(t);
  }, []);

  async function refreshSessions() {
    try {
      const sessions = await fetchSessions();
      setActiveSessions(sessions.filter((s) => s.alive).length);
    } catch {
      /* ignore */
    }
  }

  async function loadPresets() {
    setLoading(true);
    try {
      const data = await fetchPresets();
      setPresets(data);
    } catch {
      showToast('Failed to load presets', 'error');
    }
    setLoading(false);
  }

  const update = <K extends keyof ScrcpyConfig>(key: K, value: ScrcpyConfig[K]) => {
    setConfig((prev) => ({ ...prev, [key]: value }));
  };

  const handleCopy = async () => {
    try {
      await navigator.clipboard.writeText(command);
      setCopied(true);
      setTimeout(() => setCopied(false), 1500);
    } catch {
      showToast('Failed to copy', 'error');
    }
  };

  const handleSavePreset = async () => {
    if (!presetName.trim()) {
      showToast('Enter a preset name first', 'error');
      return;
    }
    try {
      await savePreset(presetName.trim(), config, command);
      showToast(supabaseEnabled ? 'Preset saved (cloud)' : 'Preset saved locally');
      setPresetName('');
      setShowSaveDialog(false);
      loadPresets();
    } catch {
      showToast('Failed to save preset', 'error');
    }
  };

  const handleLoadPreset = (preset: Preset) => {
    const loaded = { ...defaultConfig, ...preset.config };
    if (!loaded.devices) loaded.devices = [];
    loaded.devices = loaded.devices.map((d) => ({
      ...createDevice(d.serial, d.tcpip),
      ...d,
      enabled: d.enabled !== undefined ? d.enabled : true,
      useCommonSettings: d.useCommonSettings !== undefined ? d.useCommonSettings : true,
      overrides: d.overrides || {},
    }));
    setConfig(loaded);
    showToast(`Loaded "${preset.name}"`);
    setShowPresets(false);
  };

  const handleDeletePreset = async (id: string) => {
    try {
      await deletePreset(id);
      showToast('Preset deleted');
      loadPresets();
    } catch {
      showToast('Failed to delete', 'error');
    }
  };

  const handleReset = () => {
    setConfig(defaultConfig);
    showToast('Reset to defaults');
  };

  const activeCount = Object.entries(config).filter(([k, v]) => {
    if (k === 'extraArgs') return (v as string).trim() !== '';
    if (k === 'devices' || k === 'multiDevice' || k === 'serial' || k === 'tcpip') return false;
    if (typeof v === 'boolean') return v === true;
    if (v === null) return false;
    if (typeof v === 'number') return v > 0 && k !== 'rotation';
    if (typeof v === 'string') return v !== '' && v !== 'auto' && v !== 'info' && v !== 'mp4';
    return false;
  }).length;

  const addDevice = (serial = '', tcpip = '') => {
    setConfig((prev) => ({
      ...prev,
      multiDevice: true,
      devices: [...prev.devices, createDevice(serial, tcpip)],
    }));
  };

  const updateDevice = (id: string, field: 'serial' | 'tcpip' | 'enabled' | 'useCommonSettings', value: string | boolean) => {
    setConfig((prev) => ({
      ...prev,
      devices: prev.devices.map((d) => d.id === id ? { ...d, [field]: value } : d),
    }));
  };

  const updateDeviceOverride = (id: string, key: keyof ScrcpyConfig, value: ScrcpyConfig[keyof ScrcpyConfig]) => {
    setConfig((prev) => ({
      ...prev,
      devices: prev.devices.map((d) => {
        if (d.id !== id) return d;
        return { ...d, overrides: { ...d.overrides, [key]: value } };
      }),
    }));
  };

  const removeDevice = (id: string) => {
    setConfig((prev) => ({
      ...prev,
      devices: prev.devices.filter((d) => d.id !== id),
    }));
    if (editingDeviceId === id) setEditingDeviceId(null);
  };

  const handleScan = async () => {
    setScanning(true);
    setScanError(null);
    try {
      const found = await scanDevices();
      const ready = found.filter((d) => !d.state || d.state === 'device');
      if (ready.length === 0) {
        showToast('No devices found', 'error');
      } else {
        setConfig((prev) => {
          const existingSerials = new Set(prev.devices.map((d) => d.serial));
          const newDevices = ready
            .filter((dev) => !existingSerials.has(dev.serial))
            .map((dev) => createDevice(dev.serial, dev.tcpip || ''));
          return {
            ...prev,
            multiDevice: true,
            devices: [...prev.devices, ...newDevices],
          };
        });
        showToast(`Found ${ready.length} device(s)`);
      }
    } catch (e) {
      setScanError(
        e instanceof Error
          ? e.message
          : 'Cannot reach ADB. Start the GUI server (npm run server) and connect a device with USB debugging.',
      );
    } finally {
      setScanning(false);
    }
  };

  const handleRun = async () => {
    const cmds = buildCommands({ ...config, multiDevice: true });
    if (!cmds.length || (cmds.length === 1 && cmds[0] === 'scrcpy')) {
      // allow bare scrcpy if no devices configured
    }
    const toRun = cmds.filter((c) => c.trim());
    if (!toRun.length) {
      showToast('Nothing to run', 'error');
      return;
    }
    setRunning(true);
    try {
      await startCommands(toRun);
      showToast(`Started ${toRun.length} session(s)`);
      refreshSessions();
    } catch (e) {
      showToast(e instanceof Error ? e.message : 'Failed to start', 'error');
    } finally {
      setRunning(false);
    }
  };

  const handleStopAll = async () => {
    try {
      await stopSessions();
      showToast('Stopped all sessions');
      refreshSessions();
    } catch (e) {
      showToast(e instanceof Error ? e.message : 'Failed to stop', 'error');
    }
  };

  const editingDevice = config.devices.find((d) => d.id === editingDeviceId) || null;

  return (
    <div className="min-h-screen bg-slate-50 text-slate-900">
      {/* Header */}
      <header className="sticky top-0 z-30 bg-white/80 backdrop-blur-xl border-b border-slate-200">
        <div className="max-w-6xl mx-auto px-4 sm:px-6 py-3.5 flex items-center justify-between gap-4">
          <div className="flex items-center gap-3">
            <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-emerald-500 to-teal-600 flex items-center justify-center shadow-lg shadow-emerald-500/20">
              <Smartphone className="w-5 h-5 text-white" />
            </div>
            <div>
              <h1 className="text-base font-bold text-slate-800 leading-tight">scrcpy GUI</h1>
              <p className="text-xs text-slate-400 leading-tight">Multi-device Android mirroring</p>
            </div>
          </div>
          <div className="flex items-center gap-2">
            <button
              onClick={handleRun}
              disabled={running}
              className="flex items-center gap-1.5 px-3.5 py-2 rounded-xl text-sm font-semibold text-white bg-emerald-500 hover:bg-emerald-600 transition-colors shadow-sm shadow-emerald-500/20 disabled:opacity-50"
            >
              <Play className="w-4 h-4" />
              <span className="hidden sm:inline">{running ? 'Starting…' : 'Run'}</span>
            </button>
            <button
              onClick={handleStopAll}
              className="flex items-center gap-1.5 px-3 py-2 rounded-xl text-sm font-medium text-slate-600 hover:bg-slate-100 transition-colors"
              title={activeSessions ? `${activeSessions} active` : 'Stop all'}
            >
              <Square className="w-4 h-4" />
              <span className="hidden sm:inline">Stop{activeSessions ? ` (${activeSessions})` : ''}</span>
            </button>
            <button
              onClick={() => setShowPresets(true)}
              className="flex items-center gap-1.5 px-3 py-2 rounded-xl text-sm font-medium text-slate-600 hover:bg-slate-100 transition-colors"
            >
              <FolderOpen className="w-4 h-4" />
              <span className="hidden sm:inline">Presets</span>
              {presets.length > 0 && (
                <span className="text-xs bg-slate-200 text-slate-600 px-1.5 py-0.5 rounded-full">{presets.length}</span>
              )}
            </button>
            <button
              onClick={handleReset}
              className="flex items-center gap-1.5 px-3 py-2 rounded-xl text-sm font-medium text-slate-600 hover:bg-slate-100 transition-colors"
            >
              <RotateCw className="w-4 h-4" />
              <span className="hidden sm:inline">Reset</span>
            </button>
            <button
              onClick={() => setShowSaveDialog(true)}
              className="flex items-center gap-1.5 px-3.5 py-2 rounded-xl text-sm font-semibold text-slate-700 bg-slate-100 hover:bg-slate-200 transition-colors"
            >
              <Save className="w-4 h-4" />
              <span className="hidden sm:inline">Save</span>
            </button>
          </div>
        </div>
      </header>

      <main className="max-w-6xl mx-auto px-4 sm:px-6 py-6 pb-32">
        <div className="grid grid-cols-1 lg:grid-cols-5 gap-6">
          {/* Left: Controls */}
          <div className="lg:col-span-3 space-y-3">
            {/* Quick info banner */}
            <div className="flex items-center gap-3 p-4 rounded-2xl bg-gradient-to-r from-slate-800 to-slate-700 text-white">
              <div className="w-10 h-10 rounded-xl bg-white/10 flex items-center justify-center flex-shrink-0">
                <Zap className="w-5 h-5 text-emerald-400" />
              </div>
              <div className="flex-1 min-w-0">
                <p className="text-sm font-semibold">Configure your scrcpy session</p>
                <p className="text-xs text-slate-300 mt-0.5">
                  {activeCount === 0
                    ? 'All defaults — just run scrcpy as-is'
                    : `${activeCount} option${activeCount > 1 ? 's' : ''} active`}
                </p>
              </div>
              <div className="flex items-center gap-1.5 text-xs font-mono text-emerald-400 bg-white/5 px-3 py-1.5 rounded-lg flex-shrink-0">
                <Terminal className="w-3.5 h-3.5" />
                <span>v4.1</span>
              </div>
            </div>

            {/* Devices Section */}
            <div className="bg-white rounded-2xl border border-slate-200 overflow-hidden">
              <div className="flex items-center justify-between p-4">
                <div className="flex items-center gap-3">
                  <div className="w-10 h-10 rounded-xl bg-slate-100 flex items-center justify-center text-slate-600">
                    <Smartphone className="w-5 h-5" />
                  </div>
                  <div>
                    <h3 className="text-sm font-semibold text-slate-800">Devices</h3>
                    <p className="text-xs text-slate-400">Connected devices and per-device settings</p>
                  </div>
                </div>
                <button
                  onClick={handleScan}
                  disabled={scanning}
                  className="flex items-center gap-1.5 px-3 py-2 rounded-xl text-sm font-medium text-emerald-600 bg-emerald-50 hover:bg-emerald-100 transition-colors disabled:opacity-50"
                >
                  {scanning ? <RefreshCw className="w-4 h-4 animate-spin" /> : <Search className="w-4 h-4" />}
                  <span className="hidden sm:inline">{scanning ? 'Scanning...' : 'Scan Devices'}</span>
                </button>
              </div>

              {scanError && (
                <div className="mx-4 mb-3 flex items-start gap-2 p-3 rounded-xl bg-amber-50 border border-amber-200">
                  <AlertCircle className="w-4 h-4 text-amber-500 mt-0.5 flex-shrink-0" />
                  <p className="text-xs text-amber-700">{scanError}</p>
                </div>
              )}

              <div className="px-4 pb-4 space-y-3">
                {config.devices.length === 0 && (
                  <div className="flex flex-col items-center justify-center py-8 text-center">
                    <div className="w-14 h-14 rounded-2xl bg-slate-100 flex items-center justify-center mb-3">
                      <Usb className="w-6 h-6 text-slate-300" />
                    </div>
                    <p className="text-sm text-slate-400 mb-1">No devices added yet</p>
                    <p className="text-xs text-slate-400 mb-4">Scan for connected devices, or add one manually</p>
                    <div className="flex gap-2">
                      <button
                        onClick={handleScan}
                        disabled={scanning}
                        className="flex items-center gap-1.5 px-3 py-2 rounded-xl text-sm font-medium text-emerald-600 bg-emerald-50 hover:bg-emerald-100 transition-colors disabled:opacity-50"
                      >
                        {scanning ? <RefreshCw className="w-4 h-4 animate-spin" /> : <Search className="w-4 h-4" />}
                        Scan
                      </button>
                      <button
                        onClick={() => addDevice()}
                        className="flex items-center gap-1.5 px-3 py-2 rounded-xl text-sm font-medium text-slate-600 bg-slate-100 hover:bg-slate-200 transition-colors"
                      >
                        <Plus className="w-4 h-4" />
                        Add Manually
                      </button>
                    </div>
                  </div>
                )}

                {config.devices.map((device, idx) => (
                  <div key={device.id} className={`p-3.5 rounded-xl border transition-all ${
                    device.enabled
                      ? 'border-slate-200 bg-slate-50/50'
                      : 'border-slate-200 bg-slate-100/40 opacity-60'
                  }`}>
                    <div className="flex items-center justify-between mb-3">
                      <div className="flex items-center gap-2.5">
                        <button
                          onClick={() => updateDevice(device.id, 'enabled', !device.enabled)}
                          className={`relative w-9 h-5 rounded-full transition-colors duration-200 flex items-center p-0.5 ${
                            device.enabled ? 'bg-emerald-500' : 'bg-slate-300'
                          }`}
                        >
                          <div className={`w-4 h-4 rounded-full bg-white shadow-sm transition-transform duration-200 ${
                            device.enabled ? 'translate-x-4' : 'translate-x-0'
                          }`} />
                        </button>
                        <span className="text-xs font-semibold text-slate-500 uppercase tracking-wide">
                          Device {idx + 1}
                        </span>
                        {device.useCommonSettings ? (
                          <span className="text-xs font-medium text-emerald-600 bg-emerald-50 px-2 py-0.5 rounded-full">Common settings</span>
                        ) : (
                          <span className="text-xs font-medium text-blue-600 bg-blue-50 px-2 py-0.5 rounded-full">Custom settings</span>
                        )}
                        {!device.enabled && (
                          <span className="text-xs font-medium text-slate-400 bg-slate-200 px-1.5 py-0.5 rounded">off</span>
                        )}
                      </div>
                      <button
                        onClick={() => removeDevice(device.id)}
                        className="p-1.5 rounded-lg text-slate-300 hover:text-red-500 hover:bg-red-50 transition-colors"
                      >
                        <Trash2 className="w-4 h-4" />
                      </button>
                    </div>

                    <div className="grid grid-cols-2 gap-2.5">
                      <TextField label="Serial" value={device.serial} onChange={(v) => updateDevice(device.id, 'serial', v)} placeholder="e.g. emulator-5554" />
                      <TextField label="TCP/IP Address" value={device.tcpip} onChange={(v) => updateDevice(device.id, 'tcpip', v)} placeholder="e.g. 192.168.1.100:5555" />
                    </div>

                    <div className="flex items-center gap-2 mt-3">
                      <button
                        onClick={() => setEditingDeviceId(device.id)}
                        className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium text-slate-600 bg-slate-100 hover:bg-slate-200 transition-colors"
                      >
                        <Sliders className="w-3.5 h-3.5" />
                        Device Settings
                      </button>
                      <label className="flex items-center gap-2 cursor-pointer">
                        <button
                          onClick={() => updateDevice(device.id, 'useCommonSettings', !device.useCommonSettings)}
                          className={`relative w-9 h-5 rounded-full transition-colors duration-200 flex items-center p-0.5 ${
                            device.useCommonSettings ? 'bg-emerald-500' : 'bg-slate-300'
                          }`}
                        >
                          <div className={`w-4 h-4 rounded-full bg-white shadow-sm transition-transform duration-200 ${
                            device.useCommonSettings ? 'translate-x-4' : 'translate-x-0'
                          }`} />
                        </button>
                        <span className="text-xs text-slate-500">Use common settings</span>
                      </label>
                    </div>
                  </div>
                ))}

                {config.devices.length > 0 && (
                  <button
                    onClick={() => addDevice()}
                    className="flex items-center justify-center gap-2 w-full py-2.5 rounded-xl text-sm font-medium text-emerald-600 border-2 border-dashed border-emerald-300 hover:border-emerald-400 hover:bg-emerald-50/30 transition-all"
                  >
                    <Plus className="w-4 h-4" />
                    Add Device
                  </button>
                )}
              </div>
            </div>

            {/* Common Settings Section */}
            <div className="bg-white rounded-2xl border border-slate-200 overflow-hidden">
              <button
                type="button"
                onClick={() => setShowCommonSettings(!showCommonSettings)}
                className="flex items-center justify-between w-full p-4 hover:bg-slate-50/50 transition-colors"
              >
                <div className="flex items-center gap-3">
                  <div className="w-10 h-10 rounded-xl bg-emerald-100 flex items-center justify-center text-emerald-600">
                    <Sliders className="w-5 h-5" />
                  </div>
                  <div className="text-left">
                    <h3 className="text-sm font-semibold text-slate-800">Common Settings (Default)</h3>
                    <p className="text-xs text-slate-400">Applied to all devices that use common settings</p>
                  </div>
                </div>
                {showCommonSettings ? <ChevronDown className="w-5 h-5 text-slate-400" /> : <ChevronRight className="w-5 h-5 text-slate-400" />}
              </button>
              <div className={`grid transition-all duration-200 ease-in-out ${
                showCommonSettings ? 'grid-rows-[1fr] opacity-100' : 'grid-rows-[0fr] opacity-0'
              }`}>
                <div className="overflow-hidden">
                  <div className="p-4 pt-0">
                    <SettingsSections config={config} update={update} defaultOpen={['video']} />
                  </div>
                </div>
              </div>
            </div>
          </div>

          {/* Right: Command Preview */}
          <div className="lg:col-span-2">
            <div className="lg:sticky lg:top-20 space-y-4">
              {/* Command Terminal */}
              <div className="bg-slate-900 rounded-2xl overflow-hidden shadow-xl shadow-slate-900/10">
                <div className="flex items-center justify-between px-4 py-3 bg-slate-800/50 border-b border-slate-700/50">
                  <div className="flex items-center gap-2">
                    <div className="flex gap-1.5">
                      <div className="w-3 h-3 rounded-full bg-red-400/80" />
                      <div className="w-3 h-3 rounded-full bg-amber-400/80" />
                      <div className="w-3 h-3 rounded-full bg-emerald-400/80" />
                    </div>
                    <span className="text-xs text-slate-400 font-mono ml-2">terminal</span>
                  </div>
                  <button
                    onClick={handleCopy}
                    className="flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg text-xs font-medium text-slate-300 hover:bg-slate-700/50 transition-colors"
                  >
                    {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                    {copied ? 'Copied' : 'Copy'}
                  </button>
                </div>
                <div className="p-4">
                  {commands.length === 0 ? (
                    <p className="text-sm text-slate-500 font-mono">No enabled devices with a serial or TCP/IP address.</p>
                  ) : (
                    commands.map((cmd, idx) => (
                      <div key={idx} className={idx > 0 ? 'mt-3 pt-3 border-t border-slate-700/50' : ''}>
                        <pre className="text-sm font-mono text-emerald-400 break-all whitespace-pre-wrap leading-relaxed">
                          <span className="text-slate-500">$ </span>
                          {cmd}
                        </pre>
                      </div>
                    ))
                  )}
                </div>
              </div>

              {/* Quick Stats */}
              <div className="grid grid-cols-2 gap-3">
                <div className="p-4 rounded-2xl bg-white border border-slate-200">
                  <div className="flex items-center gap-2 mb-1">
                    <Layers className="w-4 h-4 text-slate-400" />
                    <span className="text-xs font-semibold text-slate-500 uppercase tracking-wide">Options</span>
                  </div>
                  <p className="text-2xl font-bold text-slate-800">{activeCount}</p>
                </div>
                <div className="p-4 rounded-2xl bg-white border border-slate-200">
                  <div className="flex items-center gap-2 mb-1">
                    <Smartphone className="w-4 h-4 text-slate-400" />
                    <span className="text-xs font-semibold text-slate-500 uppercase tracking-wide">Devices</span>
                  </div>
                  <p className="text-2xl font-bold text-slate-800">
                    {`${config.devices.filter((d) => d.enabled).length}/${config.devices.length}`}
                  </p>
                </div>
                <div className="p-4 rounded-2xl bg-white border border-slate-200">
                  <div className="flex items-center gap-2 mb-1">
                    <Eye className="w-4 h-4 text-slate-400" />
                    <span className="text-xs font-semibold text-slate-500 uppercase tracking-wide">Video</span>
                  </div>
                  <p className="text-sm font-bold text-slate-800 mt-1">
                    {config.noVideo ? 'Off' : config.maxSize ? `${config.maxSize}px` : 'Default'}
                  </p>
                </div>
                <div className="p-4 rounded-2xl bg-white border border-slate-200">
                  <div className="flex items-center gap-2 mb-1">
                    <Volume2 className="w-4 h-4 text-slate-400" />
                    <span className="text-xs font-semibold text-slate-500 uppercase tracking-wide">Audio</span>
                  </div>
                  <p className="text-sm font-bold text-slate-800 mt-1">
                    {config.noAudio ? 'Off' : config.audioCodec === 'auto' ? 'Auto' : config.audioCodec}
                  </p>
                </div>
              </div>

              {/* Tips */}
              <div className="p-4 rounded-2xl bg-emerald-50 border border-emerald-200">
                <div className="flex items-start gap-2.5">
                  <Terminal className="w-4 h-4 text-emerald-600 mt-0.5 flex-shrink-0" />
                  <div>
                    <p className="text-xs font-semibold text-emerald-800 mb-1">Quick Tips</p>
                    <ul className="text-xs text-emerald-700 space-y-1 leading-relaxed">
                      <li>Enable USB debugging on your Android device first</li>
                      <li>Use --no-audio for lower latency on older devices</li>
                      <li>H.265 codec gives better quality at same bitrate</li>
                      <li>Requires Android 5.0+ (API 21)</li>
                    </ul>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </main>

      {/* Device Settings Drawer */}
      {editingDevice && (
        <div className="fixed inset-0 z-50 flex justify-end" onClick={() => setEditingDeviceId(null)}>
          <div className="absolute inset-0 bg-slate-900/40 backdrop-blur-sm" />
          <div
            className="relative w-full max-w-lg bg-white h-full overflow-y-auto shadow-2xl"
            onClick={(e) => e.stopPropagation()}
          >
            <div className="sticky top-0 bg-white border-b border-slate-200 px-5 py-4 flex items-center justify-between z-10">
              <div className="flex items-center gap-2">
                <button onClick={() => setEditingDeviceId(null)} className="p-1.5 rounded-lg hover:bg-slate-100 transition-colors">
                  <ChevronLeft className="w-5 h-5 text-slate-400" />
                </button>
                <div>
                  <h2 className="text-lg font-bold text-slate-800">Device Settings</h2>
                  <p className="text-xs text-slate-400 font-mono">{editingDevice.serial || editingDevice.tcpip || 'Unnamed device'}</p>
                </div>
              </div>
              <button onClick={() => setEditingDeviceId(null)} className="p-1.5 rounded-lg hover:bg-slate-100 transition-colors">
                <X className="w-5 h-5 text-slate-400" />
              </button>
            </div>

            <div className="p-5 space-y-4">
              {/* Use common settings toggle */}
              <div className="p-4 rounded-2xl bg-slate-50 border border-slate-200">
                <Toggle
                  checked={editingDevice.useCommonSettings}
                  onChange={(v) => updateDevice(editingDevice.id, 'useCommonSettings', v)}
                  label="Use common settings"
                  hint="When on, this device uses the shared default settings. Turn off to customize."
                />
              </div>

              {editingDevice.useCommonSettings ? (
                <div className="flex flex-col items-center justify-center py-12 text-center">
                  <div className="w-14 h-14 rounded-2xl bg-emerald-50 flex items-center justify-center mb-3">
                    <Sliders className="w-6 h-6 text-emerald-400" />
                  </div>
                  <p className="text-sm font-medium text-slate-600">Using common settings</p>
                  <p className="text-xs text-slate-400 mt-1 max-w-xs">
                    This device inherits all settings from the Common Settings section. Turn off "Use common settings" to customize individually.
                  </p>
                </div>
              ) : (
                <SettingsSections
                  config={{ ...config, ...editingDevice.overrides }}
                  update={(key, value) => updateDeviceOverride(editingDevice.id, key, value)}
                  defaultOpen={['video']}
                />
              )}
            </div>
          </div>
        </div>
      )}

      {/* Save Dialog */}
      {showSaveDialog && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-slate-900/40 backdrop-blur-sm" onClick={() => setShowSaveDialog(false)}>
          <div className="bg-white rounded-2xl shadow-2xl w-full max-w-md p-6" onClick={(e) => e.stopPropagation()}>
            <div className="flex items-center justify-between mb-4">
              <h2 className="text-lg font-bold text-slate-800">Save Preset</h2>
              <button onClick={() => setShowSaveDialog(false)} className="p-1.5 rounded-lg hover:bg-slate-100 transition-colors">
                <X className="w-5 h-5 text-slate-400" />
              </button>
            </div>
            <TextField label="Preset Name" value={presetName} onChange={setPresetName} placeholder="e.g. High Quality Mirror" />
            <div className="mt-4 p-3 rounded-xl bg-slate-50 border border-slate-200">
              <p className="text-xs text-slate-400 mb-1 font-semibold uppercase tracking-wide">Command Preview</p>
              <p className="text-xs font-mono text-slate-600 break-all">{command}</p>
            </div>
            <div className="flex gap-3 mt-5">
              <button
                onClick={() => setShowSaveDialog(false)}
                className="flex-1 py-2.5 rounded-xl text-sm font-medium text-slate-600 bg-slate-100 hover:bg-slate-200 transition-colors"
              >
                Cancel
              </button>
              <button
                onClick={handleSavePreset}
                className="flex-1 py-2.5 rounded-xl text-sm font-semibold text-white bg-emerald-500 hover:bg-emerald-600 transition-colors shadow-sm shadow-emerald-500/20"
              >
                Save Preset
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Presets Drawer */}
      {showPresets && (
        <div className="fixed inset-0 z-50 flex justify-end" onClick={() => setShowPresets(false)}>
          <div className="absolute inset-0 bg-slate-900/40 backdrop-blur-sm" />
          <div
            className="relative w-full max-w-md bg-white h-full overflow-y-auto shadow-2xl"
            onClick={(e) => e.stopPropagation()}
          >
            <div className="sticky top-0 bg-white border-b border-slate-200 px-5 py-4 flex items-center justify-between z-10">
              <div className="flex items-center gap-2">
                <FolderOpen className="w-5 h-5 text-slate-600" />
                <h2 className="text-lg font-bold text-slate-800">Saved Presets</h2>
              </div>
              <button onClick={() => setShowPresets(false)} className="p-1.5 rounded-lg hover:bg-slate-100 transition-colors">
                <X className="w-5 h-5 text-slate-400" />
              </button>
            </div>
            <div className="p-5 space-y-3">
              {loading ? (
                <div className="flex items-center justify-center py-12">
                  <div className="w-8 h-8 border-2 border-slate-200 border-t-emerald-500 rounded-full animate-spin" />
                </div>
              ) : presets.length === 0 ? (
                <div className="flex flex-col items-center justify-center py-16 text-center">
                  <div className="w-16 h-16 rounded-2xl bg-slate-100 flex items-center justify-center mb-4">
                    <Save className="w-7 h-7 text-slate-300" />
                  </div>
                  <p className="text-sm font-medium text-slate-500">No presets yet</p>
                  <p className="text-xs text-slate-400 mt-1">Save your current configuration to reuse it later</p>
                </div>
              ) : (
                presets.map((preset) => (
                  <div
                    key={preset.id}
                    className="group p-4 rounded-2xl border border-slate-200 hover:border-emerald-300 hover:bg-emerald-50/30 transition-all cursor-pointer"
                    onClick={() => handleLoadPreset(preset)}
                  >
                    <div className="flex items-start justify-between gap-3">
                      <div className="flex-1 min-w-0">
                        <h3 className="text-sm font-semibold text-slate-800">{preset.name}</h3>
                        <p className="text-xs font-mono text-slate-400 mt-1 break-all line-clamp-2">{preset.command}</p>
                        <p className="text-xs text-slate-300 mt-1.5">
                          {new Date(preset.updated_at).toLocaleDateString(undefined, { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit' })}
                        </p>
                      </div>
                      <button
                        onClick={(e) => { e.stopPropagation(); handleDeletePreset(preset.id); }}
                        className="p-2 rounded-lg text-slate-300 hover:text-red-500 hover:bg-red-50 transition-colors opacity-0 group-hover:opacity-100"
                      >
                        <Trash2 className="w-4 h-4" />
                      </button>
                    </div>
                  </div>
                ))
              )}
            </div>
          </div>
        </div>
      )}

      {/* Toast */}
      {toast && (
        <div className="fixed bottom-6 left-1/2 -translate-x-1/2 z-50 animate-in fade-in slide-in-from-bottom-4 duration-200">
          <div className={`flex items-center gap-2.5 px-4 py-3 rounded-xl shadow-lg ${
            toast.type === 'success' ? 'bg-emerald-500 text-white' : 'bg-red-500 text-white'
          }`}>
            {toast.type === 'success' ? <CheckCircle2 className="w-4 h-4" /> : <AlertCircle className="w-4 h-4" />}
            <span className="text-sm font-medium">{toast.msg}</span>
          </div>
        </div>
      )}
    </div>
  );
}

export default App;
