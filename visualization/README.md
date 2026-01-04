# LTLf Synthesis Trace Visualizer

Web-based visualization tool for LTLf synthesis execution traces.

## Features

- **Step Tree**: Left sidebar showing execution stages (expand, SCC, fixed-point)
- **Graph Canvas**: Main display area with DOT graph rendering using viz.js
- **Stepper Controls**: Bottom navigation with prev/next, auto-play, and speed control
- **Highlights**: Visual indicators for new nodes, SCCs, and state changes
- **Keyboard Shortcuts**: Arrow keys for navigation, Space for play/pause

## Development

### Install Dependencies

```bash
npm install
```

### Run Development Server

```bash
npm run dev
```

### Build for Production

```bash
npm run build
```

### Preview Production Build

```bash
npm run preview
```

## Usage

1. Generate trace JSON from Cosy2 with `--trace` option
2. Open the visualizer in a browser
3. Load the trace JSON file using the "Load Trace JSON" button
4. Navigate through steps using:
   - Click on steps in the sidebar
   - Use prev/next buttons
   - Press arrow keys
   - Enable auto-play with space bar

## Project Structure

```
src/
├── components/
│   ├── GraphCanvas.vue    # DOT graph display
│   ├── StepTree.vue       # Stage/sub-step tree
│   └── Stepper.vue        # Navigation controls
├── composables/
│   └── useGraphViz.ts     # Graphviz rendering composable
├── types/
│   └── trace.ts           # TypeScript type definitions
├── App.vue                # Main application
├── main.ts                # Entry point
└── style.css             # Global styles
```

## Tech Stack

- **Vue 3** - Frontend framework
- **TypeScript** - Type safety
- **Vite** - Build tool
- **@viz-js/viz** - Graphviz DOT rendering
