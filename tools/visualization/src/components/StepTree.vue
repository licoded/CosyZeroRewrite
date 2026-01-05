<!--
  StepTree.vue
  Component for displaying the execution step tree (stages and sub-steps)
-->
<template>
  <div class="step-tree">
    <div v-if="!trace || trace.stages.length === 0" class="empty">
      No trace data available
    </div>

    <div v-else class="tree-content" ref="treeContentRef">
      <div
        v-for="(stage, stageIndex) in trace.stages"
        :key="stage.stage_id"
        class="stage"
      >
        <div
          class="stage-header"
          :class="{ active: activeStage === stageIndex }"
          @click="toggleStage(stageIndex)"
        >
          <span class="stage-toggle">{{ expandedStages.has(stageIndex) ? '▼' : '▶' }}</span>
          <span class="stage-type">{{ stageTypeDisplay(stage.stage_type) }}</span>
          <span class="stage-count">({{ stage.sub_steps.length }} steps)</span>
        </div>

        <div
          v-if="expandedStages.has(stageIndex)"
          class="sub-steps"
        >
          <div
            v-for="(step, stepIndex) in stage.sub_steps"
            :key="step.step_id"
            class="sub-step"
            :class="{
              active: isActiveStep(stageIndex, stepIndex),
              selected: selectedStepId === step.step_id
            }"
            @click="selectStep(stageIndex, stepIndex)"
          >
            <span class="step-indicator">{{ getStepIndicator(stageIndex, stepIndex) }}</span>
            <span class="step-description">{{ step.description || 'Step ' + stepIndex }}</span>
            <span class="step-time">{{ formatDuration(step.metrics.duration_ms) }}</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, nextTick } from 'vue';
import type { Trace, StageType } from '@/types/trace';

interface Props {
  trace: Trace | null;
  modelValue: number;  // Global step index
}

const props = defineProps<Props>();

const emit = defineEmits<{
  'update:modelValue': [value: number];
  'selectStage': [stageIndex: number];
  'selectStep': [stageIndex: number, stepIndex: number];
}>();

const expandedStages = ref<Set<number>>(new Set([0]));  // First stage expanded by default
const activeStage = ref<number>(0);
const treeContentRef = ref<HTMLElement | null>(null);

// Auto-expand the stage containing the current step and scroll to it (2026-01-04)
watch(() => props.modelValue, (newGlobalIndex) => {
  if (!props.trace || newGlobalIndex < 0) return;

  // Find which stage contains this step
  let accumulatedSteps = 0;
  for (let s = 0; s < props.trace.stages.length; s++) {
    const stageStepCount = props.trace.stages[s].sub_steps.length;
    if (newGlobalIndex >= accumulatedSteps && newGlobalIndex < accumulatedSteps + stageStepCount) {
      // This is the stage containing the current step
      activeStage.value = s;
      // Auto-expand this stage
      expandedStages.value.add(s);

      // Scroll to the active step element after DOM update
      nextTick(() => {
        const stepIndex = newGlobalIndex - accumulatedSteps;
        scrollActiveStepIntoView(s, stepIndex);
      });
      break;
    }
    accumulatedSteps += stageStepCount;
  }
}, { immediate: true });

// Scroll the active step into view
function scrollActiveStepIntoView(_stageIndex: number, _stepIndex: number): void {
  if (!treeContentRef.value) return;

  // Find all active sub-step elements
  const activeSteps = treeContentRef.value.querySelectorAll('.sub-step.active');
  if (activeSteps.length === 0) return;

  // Get the first (should be only one) active step element
  const activeElement = activeSteps[0] as HTMLElement;

  // Scroll into view with smooth behavior, centered if possible
  activeElement.scrollIntoView({
    behavior: 'smooth',
    block: 'nearest'
  });
}

// Flatten all steps to get global index
const flatSteps = computed(() => {
  const steps: Array<{ stageIndex: number; stepIndex: number; stepId: string }> = [];
  if (!props.trace) return steps;

  let globalIndex = 0;
  for (let s = 0; s < props.trace.stages.length; s++) {
    for (let st = 0; st < props.trace.stages[s].sub_steps.length; st++) {
      steps.push({
        stageIndex: s,
        stepIndex: st,
        stepId: props.trace.stages[s].sub_steps[st].step_id
      });
      globalIndex++;
    }
  }
  return steps;
});

const selectedStepId = computed(() => {
  if (props.modelValue >= 0 && props.modelValue < flatSteps.value.length) {
    const step = flatSteps.value[props.modelValue];
    return step?.stepId ?? '';
  }
  return '';
});

function stageTypeDisplay(type: StageType): string {
  const display: Record<StageType, string> = {
    expand: 'Expand',
    scc: 'SCC',
    fixed_point: 'Fixed Point',
    attractor: 'Attractor'
  };
  return display[type] || type;
}

function toggleStage(stageIndex: number): void {
  if (expandedStages.value.has(stageIndex)) {
    expandedStages.value.delete(stageIndex);
  } else {
    expandedStages.value.add(stageIndex);
  }
}

function selectStep(stageIndex: number, stepIndex: number): void {
  activeStage.value = stageIndex;

  // Find global index
  let globalIndex = 0;
  for (let s = 0; s < stageIndex; s++) {
    if (props.trace) {
      globalIndex += props.trace.stages[s].sub_steps.length;
    }
  }
  globalIndex += stepIndex;

  emit('update:modelValue', globalIndex);
  emit('selectStep', stageIndex, stepIndex);
}

function isActiveStep(stageIndex: number, stepIndex: number): boolean {
  if (props.modelValue < 0 || !flatSteps.value) return false;
  const current = flatSteps.value[props.modelValue];
  return current?.stageIndex === stageIndex && current?.stepIndex === stepIndex;
}

function getStepIndicator(stageIndex: number, stepIndex: number): string {
  let globalIndex = 0;
  for (let s = 0; s < stageIndex; s++) {
    if (props.trace) {
      globalIndex += props.trace.stages[s].sub_steps.length;
    }
  }
  globalIndex += stepIndex;
  return (globalIndex + 1).toString();
}

function formatDuration(ms: number): string {
  if (ms < 1) return '< 1ms';
  if (ms < 1000) return `${Math.round(ms)}ms`;
  return `${(ms / 1000).toFixed(1)}s`;
}
</script>

<style scoped>
.step-tree {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #f8f9fa;
  border-radius: 8px;
  overflow: hidden;
}

.empty {
  padding: 20px;
  text-align: center;
  color: #6c757d;
  font-style: italic;
}

.tree-content {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
}

.stage {
  margin-bottom: 8px;
}

.stage-header {
  display: flex;
  align-items: center;
  padding: 8px 12px;
  background: white;
  border-radius: 6px;
  cursor: pointer;
  user-select: none;
  transition: background 0.2s;
}

.stage-header:hover {
  background: #e9ecef;
}

.stage-header.active {
  background: #e7f3ff;
  border-left: 3px solid #2196F3;
}

.stage-toggle {
  margin-right: 8px;
  font-size: 10px;
  color: #6c757d;
}

.stage-type {
  font-weight: 600;
  color: #495057;
}

.stage-count {
  margin-left: auto;
  font-size: 12px;
  color: #6c757d;
}

.sub-steps {
  margin-left: 16px;
  margin-top: 4px;
}

.sub-step {
  display: flex;
  align-items: center;
  padding: 6px 12px;
  margin: 2px 0;
  background: white;
  border-radius: 4px;
  cursor: pointer;
  user-select: none;
  transition: all 0.2s;
}

.sub-step:hover {
  background: #e9ecef;
}

.sub-step.active {
  background: #2196F3;
  color: white;
}

.sub-step.selected {
  border-left: 3px solid #FF9800;
}

.step-indicator {
  width: 24px;
  height: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #dee2e6;
  border-radius: 50%;
  font-size: 11px;
  font-weight: 600;
  margin-right: 8px;
  flex-shrink: 0;
}

.sub-step.active .step-indicator {
  background: rgba(255, 255, 255, 0.3);
  color: white;
}

.step-description {
  flex: 1;
  font-size: 13px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.step-time {
  margin-left: 8px;
  font-size: 11px;
  color: #6c757d;
}

.sub-step.active .step-time {
  color: rgba(255, 255, 255, 0.8);
}
</style>
