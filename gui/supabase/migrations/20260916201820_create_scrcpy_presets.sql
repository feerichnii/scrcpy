/*
# Create scrcpy_presets table (single-tenant, no auth)

1. New Tables
- `scrcpy_presets`
- `id` (uuid, primary key)
- `name` (text, not null) — user-given name for the preset
- `config` (jsonb, not null) — full scrcpy option configuration as JSON
- `command` (text, not null) — the generated scrcpy command string
- `created_at` (timestamp, default now)
- `updated_at` (timestamp, default now)
2. Security
- Enable RLS on `scrcpy_presets`.
- Allow anon + authenticated CRUD because the data is intentionally shared/public (no-auth utility app).
*/

CREATE TABLE IF NOT EXISTS scrcpy_presets (
  id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  name text NOT NULL,
  config jsonb NOT NULL DEFAULT '{}'::jsonb,
  command text NOT NULL DEFAULT 'scrcpy',
  created_at timestamptz DEFAULT now(),
  updated_at timestamptz DEFAULT now()
);

ALTER TABLE scrcpy_presets ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "anon_select_presets" ON scrcpy_presets;
CREATE POLICY "anon_select_presets" ON scrcpy_presets FOR SELECT
  TO anon, authenticated USING (true);

DROP POLICY IF EXISTS "anon_insert_presets" ON scrcpy_presets;
CREATE POLICY "anon_insert_presets" ON scrcpy_presets FOR INSERT
  TO anon, authenticated WITH CHECK (true);

DROP POLICY IF EXISTS "anon_update_presets" ON scrcpy_presets;
CREATE POLICY "anon_update_presets" ON scrcpy_presets FOR UPDATE
  TO anon, authenticated USING (true) WITH CHECK (true);

DROP POLICY IF EXISTS "anon_delete_presets" ON scrcpy_presets;
CREATE POLICY "anon_delete_presets" ON scrcpy_presets FOR DELETE
  TO anon, authenticated USING (true);
