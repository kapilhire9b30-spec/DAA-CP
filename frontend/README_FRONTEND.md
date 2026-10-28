# CampusAlloc Frontend Web Interface (Phase 4a Restyle)

This is the React web application for the **Campus Resource Allocation Optimizer (CampusAlloc)**. Built with React 18, Vite, TypeScript, and a custom warm organic design system (zero external runtime dependencies, zero external font links, offline-ready). It interfaces directly with the Spring Boot REST API documented in `backend/README_API.md`.

---

## Phase 4a Visual Restyle & Design System

The application features a complete presentation-only visual redesign:
- **Zero Blue or Purple Banned Colors**: The UI completely eliminates blues, indigos, violets, purples, cyans, and cool slate greys across backgrounds, surfaces, text, links, buttons, borders, focus rings, scrollbars, charts, and illustrations.
- **Warm Light Palette**:
  - Background: `--bg #FAF7F2`
  - Surfaces: `--surface #FFFFFF`, `--surface-2 #F3EEE6`
  - Warm Borders: `--border #E3DACB`, `--border-strong #CDC2B0`
  - Text: `--text #1F1C17`, `--text-muted #6B6458`
  - Primary Action: `--primary #1F6F54`, `--primary-hover #185A44`, `--on-primary #FFFFFF`
  - Accent: `--accent #C9831A` (warm saffron, dark text on fills)
- **Deep Warm Dark Palette**:
  - Background: `--bg #151310`
  - Surfaces: `--surface #1E1B17`, `--surface-2 #26221D`
  - Borders: `--border #37312A`
  - Text: `--text #F1ECE3`, `--text-muted #A89F90`
  - Primary Action: `--primary #4FB38A`, `--on-primary #151310`
  - Accent: `--accent #E0A24A`
- **Semantic Status Badges (WCAG AA Compliant in both themes)**:
  - `Pending`: Stone (`#EDE7DC` / `#5C5446` in light; `#2B2620` / `#D5CCC0` in dark)
  - `Recommended`: Amber (`#FBEBC8` / `#7A4A05` in light; `#382A14` / `#F8CE84` in dark)
  - `Confirmed`: Green (`#D9EEDF` / `#15563B` in light; `#193427` / `#74CFA1` in dark)
  - `Rejected`: Brick (`#F6D9D2` / `#8A2E1B` in light; `#381D18` / `#F49E8E` in dark)
- **Schedule Timeline Room Colors**:
  - Auditorium terracotta `#B4532A`
  - Seminar Hall forest `#1F6F54`
  - Room A saffron `#C9831A`
  - Room B olive `#6F7A2C`
  - Assigned by room index, cycling through fixed warm palette for other rooms.
  - Confirmed bookings are solid blocks; suggestions are drawn with dashed outlines and light hatched fills.
- **Typographic System**:
  - Headings: `Fraunces` variable font (bundled locally via `@fontsource-variable/fraunces`), weight 600, line-height 1.2, letter-spacing -0.01em.
  - Body & UI: `Source Sans 3` variable font (bundled locally via `@fontsource-variable/source-sans-3`), weight 400, line-height 1.5; UI labels and buttons weight 600.
  - Large dashboard numbers: Fraunces, 4xl (`2.375rem`), with `font-variant-numeric: tabular-nums`.
  - Tabular numerics applied to all times, dates, IDs, and table numbers.
  - Prose line length capped at ~70ch. Minimum interactive target of 40px for touch/pointer accessibility.
- **Focus Rings**: `2px solid var(--accent)` with `2px` offset.

---

## Prerequisites

- Node.js (v18+)
- npm (v9+)
- Spring Boot backend running on `http://localhost:8080`

## Build and Run Commands

All commands are executed inside the `frontend/` directory:

```bash
cd frontend
```

### 1. Install Dependencies
```bash
npm install
```

### 2. Run Development Server
```bash
npm run dev
```
Runs the Vite development server on `http://localhost:5173`. All requests to `/api/*` are reverse-proxied to `http://localhost:8080/api/*` via Vite's proxy configuration in `vite.config.ts`.

### 3. Production Build & Type Checking
```bash
npm run build
```
Type-checks TypeScript code (`tsc -b`) and produces optimized production assets in `frontend/dist/`.

---

## Seeded Demo Accounts

The backend seeds the following user accounts upon first initialization:

| Role | Username | Password | Organization / Notes |
| :--- | :--- | :--- | :--- |
| **Administrator** | `admin` | `admin123` | Institutional Campus Administrator |
| **Club User** | `robotics` | `robotics123` | Robotics Club |
| **Club User** | `coding` | `coding123` | Coding Club |
| **Club User** | `literary` | `literary123` | Literary Society |
