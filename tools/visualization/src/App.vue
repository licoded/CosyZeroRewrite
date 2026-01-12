<!--
  App.vue
  Main application component for LTLf Synthesis Trace Visualizer
-->
<template>
  <div class="app">
    <!-- Header -->
    <header class="header">
      <h1>LTLf Synthesis Trace Visualizer</h1>
      <div v-if="trace" class="header-info">
        <span class="formula">{{ trace.formula }}</span>
        <span class="summary">
          {{ trace.stages.length }} stages ·
          {{ totalSteps }} steps ·
          <span v-if="actualRealizable !== null" :class="{ success: actualRealizable, failure: !actualRealizable }">
            {{ actualRealizable ? 'REALIZABLE' : 'UNREALIZABLE' }}
          </span>
          <span v-else :class="{ success: trace.summary?.realizable, failure: !trace.summary?.realizable }">
            {{ trace.summary?.realizable ? 'REALIZABLE' : 'UNREALIZABLE' }}
          </span>
        </span>
      </div>
      <div v-if="trace && trace.partition" class="partition-row">
        <div class="partition-line">
          <span class="partition-label">Inputs:</span>
          <span class="partition-vars">{{ trace.partition.inputs.length > 0 ? trace.partition.inputs.join(', ') : '(none)' }}</span>
        </div>
        <div class="partition-line">
          <span class="partition-label">Outputs:</span>
          <span class="partition-vars">{{ trace.partition.outputs.join(', ') }}</span>
        </div>
      </div>
      <div v-else class="no-data">
        <label for="fileInput" class="file-label">Load Trace JSON</label>
        <input
          style="display: none;"
          id="fileInput"
          type="file"
          accept=".json"
          @change="loadFile"
          ref="fileInput"
        >
      </div>
    </header>

    <!-- Main Content -->
    <main class="main" v-if="trace">
      <!-- Left Sidebar: Step Tree -->
      <aside class="sidebar">
        <StepTree
          :trace="trace"
          v-model="currentStep"
          @select-step="handleSelectStep"
        />
      </aside>

      <!-- Center: Graph Canvas -->
      <section class="canvas-container">
        <div v-if="currentSubStep" class="canvas-wrapper">
          <GraphCanvas
            :graph-data="currentSubStep.graph_data"
            :highlights="currentSubStep.highlights"
            :partition="trace.partition"
          />
          <div class="step-info-bar">
            <span class="step-stage">{{ getStageType(currentStageIndex) }}</span>
            <span class="step-desc">{{ currentSubStep.description || 'Step ' + currentStep }}</span>
            <span class="step-metrics">
              Swin: {{ currentSubStep.state_info.swin_count }} |
              Ewin: {{ currentSubStep.state_info.ewin_count }} |
              Unknown: {{ currentSubStep.state_info.unknown_count }} |
              Time: {{ formatDuration(currentSubStep.metrics.duration_ms) }}
            </span>
          </div>
        </div>
      </section>
    </main>

    <!-- Footer: Stepper Controls -->
    <footer class="footer" v-if="trace">
      <Stepper
        :current-step="currentStep"
        :total-steps="totalSteps"
        :steps="flatSteps"
        @prev="prevStep"
        @next="nextStep"
      />
    </footer>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import StepTree from './components/StepTree.vue';
import GraphCanvas from './components/GraphCanvas.vue';
import Stepper from './components/Stepper.vue';
import type { Trace, SubStep, StageType } from './types/trace';

const trace = ref<Trace | null>(null);
const currentStep = ref(0);

// Flatten all sub-steps for stepper
const flatSteps = computed(() => {
  if (!trace.value) return [];
  return trace.value.stages.map(s => s.sub_steps);
});

const totalSteps = computed(() => {
  return flatSteps.value.reduce((sum, steps) => sum + steps.length, 0);
});

// Get the final sub-step (for determining actual realizability result)
const finalSubStep = computed((): SubStep | null => {
  if (!trace.value || totalSteps.value === 0) return null;

  let stepCount = 0;
  for (const stage of trace.value.stages) {
    for (const step of stage.sub_steps) {
      if (stepCount === totalSteps.value - 1) {
        return step;
      }
      stepCount++;
    }
  }
  return null;
});

// Get actual realizability from final step's initial state classification
const actualRealizable = computed((): boolean | null => {
  if (!finalSubStep.value?.graph_data?.state_data) return null;

  // Find the initial state (S0 or similar)
  for (const [stateId, stateData] of Object.entries(finalSubStep.value.graph_data.state_data)) {
    if (stateData.is_initial) {
      return stateData.classification === 'Swin';
    }
  }
  return null;
});

// Get current sub-step
const currentSubStep = computed((): SubStep | null => {
  if (!trace.value) return null;

  let stepCount = 0;
  for (const stage of trace.value.stages) {
    for (const step of stage.sub_steps) {
      if (stepCount === currentStep.value) {
        return step;
      }
      stepCount++;
    }
  }
  return null;
});

// Get current stage index
const currentStageIndex = computed((): number => {
  if (!trace.value) return 0;

  let stepCount = 0;
  for (let i = 0; i < trace.value.stages.length; i++) {
    if (stepCount + trace.value.stages[i].sub_steps.length > currentStep.value) {
      return i;
    }
    stepCount += trace.value.stages[i].sub_steps.length;
  }
  return 0;
});

function loadFile(event: Event): void {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  if (!file) return;

  const reader = new FileReader();
  reader.onload = (e) => {
    try {
      const json = JSON.parse(e.target?.result as string);
      trace.value = json as Trace;
      // Default to final step
      const total = flatSteps.value.reduce((sum: number, steps: SubStep[]) => sum + steps.length, 0);
      currentStep.value = total > 0 ? total - 1 : 0;
    } catch (err) {
      console.error('Failed to parse JSON:', err);
      alert('Failed to parse trace JSON file');
    }
  };
  reader.readAsText(file);
}

function handleSelectStep(stageIndex: number, stepIndex: number): void {
  if (!trace.value) return;
  let globalIndex = 0;
  for (let i = 0; i < stageIndex; i++) {
    globalIndex += trace.value.stages[i].sub_steps.length;
  }
  globalIndex += stepIndex;
  currentStep.value = globalIndex;
}

function prevStep(): void {
  if (currentStep.value > 0) {
    currentStep.value--;
  }
}

function nextStep(): void {
  if (currentStep.value < totalSteps.value - 1) {
    currentStep.value++;
  }
}

function getStageType(index: number): string {
  if (!trace.value?.stages || index >= trace.value.stages.length) return '';
  return trace.value.stages[index].stage_type;
}

function formatDuration(ms: number): string {
  if (ms < 1) return '< 1ms';
  if (ms < 1000) return `${Math.round(ms)}ms`;
  return `${(ms / 1000).toFixed(1)}s`;
}

// Keyboard navigation
function handleKeydown(e: KeyboardEvent): void {
  // Only handle if not typing in an input
  if ((e.target as HTMLElement).tagName === 'INPUT') return;

  if (e.key === 'ArrowLeft') {
    prevStep();
  } else if (e.key === 'ArrowRight') {
    nextStep();
  } else if (e.key === 'Home') {
    currentStep.value = 0;
  } else if (e.key === 'End') {
    currentStep.value = totalSteps.value - 1;
  }
}

// Demo data for development
// eslint-disable-next-line @typescript-eslint/no-unused-vars
function loadDemoData(): void {
  trace.value = {
    formula: "(true U p)",
    timestamp: new Date().toISOString(),
    stages: [
      {
        stage_id: "stage_000",
        stage_type: "expand" as StageType,
        description: "State Expansion Phase",
        sub_steps: [
          {
            step_id: "step_000",
            description: "Initial state",
            graph_data: {
              dot: 'digraph GameGraph { rankdir=LR; node[style=filled]; S0[shape=circle,fillcolor=lightcoral,color=blue,label="S0\\nEwin"]; }',
              num_nodes: 1,
              num_edges: 0,
              state_data: {
                "S0": {
                  id: "S0",
                  classification: "Ewin",
                  type: "System",
                  is_initial: true,
                  phi: "(true U p)",
                  xnf_phi: "p | (X (true U p))",
                  prop_atoms: ["p"]
                }
              }
            },
            highlights: { new_nodes: ["S0"] },
            state_info: { swin_count: 0, ewin_count: 1, unknown_count: 0, total_states: 1 },
            metrics: { duration_ms: 1.5 }
          }
        ]
      },
      {
        stage_id: "stage_001",
        stage_type: "scc" as StageType,
        description: "SCC Detection",
        sub_steps: [
          {
            step_id: "step_001",
            description: "Found SCC with 3 states",
            graph_data: {
              dot: 'digraph GameGraph { rankdir=LR; node[style=filled]; S0[shape=circle,fillcolor=lightgreen,color=blue,label="S0\\nSwin"]; S1[shape=circle,fillcolor=lightgreen,color=blue,label="S1\\nSwin"]; S2[shape=box,fillcolor=lightcoral,color=orange,label="E2\\nEwin"]; S0->S2[color=red,style=dashed]; S2->S1[color=blue,style=solid]; S1->S0[color=blue,style=solid]; }',
              num_nodes: 3,
              num_edges: 3,
              state_data: {
                "S0": {
                  id: "S0",
                  classification: "Swin",
                  type: "System",
                  is_initial: true,
                  phi: "(true U p)",
                  xnf_phi: "p | (X (true U p))",
                  prop_atoms: ["p"]
                },
                "S1": {
                  id: "S1",
                  classification: "Swin",
                  type: "System",
                  is_initial: false,
                  phi: "(true U p)",
                  xnf_phi: "p | (X (true U p))",
                  prop_atoms: ["p"]
                },
                "S2": {
                  id: "S2",
                  classification: "Ewin",
                  type: "Environment",
                  is_initial: false,
                  phi: "(true U p)",
                  xnf_phi: "p | (X (true U p))",
                  prop_atoms: ["p"]
                }
              }
            },
            highlights: { scc_nodes: ["S0", "S1", "S2"], scc_id: "scc_0" },
            state_info: { swin_count: 2, ewin_count: 1, unknown_count: 0, total_states: 3 },
            metrics: { duration_ms: 5.2 }
          }
        ]
      }
    ],
    summary: {
      total_steps: 2,
      total_states: 3,
      total_sccs: 1,
      realizable: true,
      duration_ms: 10.5,
      stages_summary: [
        { stage_type: "expand", steps_count: 1 },
        { stage_type: "scc", steps_count: 1 }
      ]
    }
  };
}

// Load demo data on mount for development
import { onMounted } from 'vue';
onMounted(() => {
  window.addEventListener('keydown', handleKeydown);
  // Uncomment for development/demo:
  // loadDemoData();
});

import { onUnmounted } from 'vue';
onUnmounted(() => {
  window.removeEventListener('keydown', handleKeydown);
});
</script>

<style>
* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

.app {
  width: 100vw;
  height: 100vh;
  display: flex;
  flex-direction: column;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: #f5f5f5;
}

.header {
  background: white;
  padding: 16px 24px;
  border-bottom: 1px solid #dee2e6;
  flex-shrink: 0;
}

.header h1 {
  font-size: 20px;
  font-weight: 600;
  color: #212529;
  margin-bottom: 8px;
}

.header-info {
  display: flex;
  align-items: center;
  gap: 16px;
  flex-wrap: wrap;
}

.formula {
  font-family: monospace;
  color: #1976D2;
  background: #E3F2FD;
  padding: 4px 8px;
  border-radius: 4px;
}

.partition-row {
  font-size: 13px;
  color: #495057;
  padding: 6px 0;
  margin-top: 4px;
}

.partition-line {
  display: flex;
  align-items: center;
  gap: 8px;
}

.partition-label {
  color: #6C757D;
  font-weight: 500;
}

.partition-vars {
  color: #212529;
  font-family: monospace;
}

.summary {
  font-size: 14px;
  color: #6c757d;
}

.summary .success {
  color: #228B22;
  font-weight: 600;
}

.summary .failure {
  color: #DC3545;
  font-weight: 600;
}

.no-data {
  display: flex;
  align-items: center;
  gap: 12px;
}

.file-label {
  padding: 8px 16px;
  background: #2196F3;
  color: white;
  border-radius: 6px;
  cursor: pointer;
}

.file-label:hover {
  background: #1976D2;
}

.main {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.sidebar {
  width: 280px;
  padding: 12px;
  border-right: 1px solid #dee2e6;
  overflow: hidden;
  flex-shrink: 0;
}

.canvas-container {
  flex: 1;
  padding: 12px;
  overflow: hidden;
  display: flex;
  flex-direction: column;
}

.canvas-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: white;
  border-radius: 8px;
  overflow: hidden;
}

.step-info-bar {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 8px 16px;
  background: #f8f9fa;
  border-top: 1px solid #dee2e6;
  font-size: 13px;
}

.step-stage {
  padding: 2px 8px;
  background: #E9ECEF;
  border-radius: 4px;
  font-weight: 600;
  font-size: 11px;
  text-transform: uppercase;
}

.step-desc {
  flex: 1;
  color: #495057;
}

.step-metrics {
  color: #6c757d;
}

.footer {
  padding: 12px 24px;
  background: white;
  border-top: 1px solid #dee2e6;
  flex-shrink: 0;
}

@media (max-width: 900px) {
  .sidebar {
    display: none;
  }
}
</style>
