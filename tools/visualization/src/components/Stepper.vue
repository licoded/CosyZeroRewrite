<!--
  Stepper.vue
  Component for step navigation controls (previous/next/auto-play)
-->
<template>
  <div class="stepper">
    <div class="stepper-section">
      <button
        class="stepper-btn"
        :disabled="currentStep <= 0"
        @click="$emit('prev')"
        title="Previous step (←)"
      >
        ← Prev
      </button>

      <div class="step-info">
        <span class="step-count">Step: {{ currentStep + 1 }} / {{ totalSteps }}</span>
        <span v-if="currentStepInfo" class="step-description">
          {{ currentStepInfo.description || '' }}
        </span>
      </div>

      <button
        class="stepper-btn"
        :disabled="currentStep >= totalSteps - 1"
        @click="$emit('next')"
        title="Next step (→)"
      >
        Next →
      </button>
    </div>

    <div class="stepper-section">
      <button
        class="play-btn"
        :class="{ playing: isPlaying }"
        @click="togglePlay"
        :title="isPlaying ? 'Pause (Space)' : 'Auto Play (Space)'"
      >
        {{ isPlaying ? '❚❚' : '▶' }}
      </button>

      <div class="speed-control" v-if="isPlaying">
        <span class="speed-label">Speed:</span>
        <input
          type="range"
          v-model.number="speed"
          min="100"
          max="2000"
          step="100"
          class="speed-slider"
          title="Playback speed (ms per step)"
        >
        <span class="speed-value">{{ speed }}ms</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, computed, onMounted, onUnmounted } from 'vue';
import type { SubStep } from '@/types/trace';

interface Props {
  currentStep: number;
  totalSteps: number;
  steps: SubStep[][];
}

const props = defineProps<Props>();

const emit = defineEmits<{
  'update:currentStep': [value: number];
  'prev': [];
  'next': [];
}>();

const isPlaying = ref(false);
const speed = ref(500);  // ms per step
let playTimer: number | null = null;

const currentStepInfo = computed(() => {
  if (props.currentStep >= 0 && props.currentStep < props.steps.length) {
    const flatSteps = props.steps.flat();
    return flatSteps[props.currentStep];
  }
  return null;
});

function togglePlay(): void {
  isPlaying.value = !isPlaying.value;

  if (isPlaying.value) {
    startPlay();
  } else {
    stopPlay();
  }
}

function startPlay(): void {
  if (playTimer !== null) return;

  playTimer = window.setInterval(() => {
    if (props.currentStep >= props.totalSteps - 1) {
      // Auto-pause at end
      isPlaying.value = false;
      stopPlay();
    } else {
      emit('next');
    }
  }, speed.value);
}

function stopPlay(): void {
  if (playTimer !== null) {
    clearInterval(playTimer);
    playTimer = null;
  }
}

// Watch for speed changes
watch(speed, () => {
  if (isPlaying.value) {
    stopPlay();
    startPlay();
  }
});

// Watch for external current step changes (e.g., user clicks in tree)
watch(() => props.currentStep, (newVal) => {
  if (newVal >= props.totalSteps - 1) {
    isPlaying.value = false;
    stopPlay();
  }
});

// Keyboard shortcuts
function handleKeydown(e: KeyboardEvent): void {
  if (e.key === 'ArrowLeft') {
    e.preventDefault();
    emit('prev');
  } else if (e.key === 'ArrowRight') {
    e.preventDefault();
    emit('next');
  } else if (e.key === ' ') {
    e.preventDefault();
    togglePlay();
  }
}

onMounted(() => {
  window.addEventListener('keydown', handleKeydown);
});

onUnmounted(() => {
  window.removeEventListener('keydown', handleKeydown);
  stopPlay();
});
</script>

<style scoped>
.stepper {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: white;
  border-radius: 8px;
  gap: 16px;
  flex-wrap: wrap;
}

.stepper-section {
  display: flex;
  align-items: center;
  gap: 12px;
}

.stepper-btn {
  padding: 8px 16px;
  border: 1px solid #dee2e6;
  background: white;
  border-radius: 6px;
  cursor: pointer;
  font-size: 14px;
  transition: all 0.2s;
}

.stepper-btn:hover:not(:disabled) {
  background: #f8f9fa;
  border-color: #adb5bd;
}

.stepper-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.step-info {
  display: flex;
  flex-direction: column;
  min-width: 120px;
}

.step-count {
  font-weight: 600;
  color: #495057;
}

.step-description {
  font-size: 12px;
  color: #6c757d;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 300px;
}

.play-btn {
  width: 40px;
  height: 40px;
  border: none;
  background: #2196F3;
  color: white;
  border-radius: 50%;
  cursor: pointer;
  font-size: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.2s;
}

.play-btn:hover {
  background: #1976D2;
}

.play-btn.playing {
  background: #FF9800;
}

.play-btn.playing:hover {
  background: #F57C00;
}

.speed-control {
  display: flex;
  align-items: center;
  gap: 8px;
  padding-left: 12px;
  border-left: 1px solid #dee2e6;
}

.speed-label {
  font-size: 12px;
  color: #6c757d;
}

.speed-slider {
  width: 100px;
  cursor: pointer;
}

.speed-value {
  font-size: 12px;
  color: #6c757d;
  min-width: 45px;
  text-align: right;
}

@media (max-width: 600px) {
  .stepper {
    flex-direction: column;
  }

  .step-description {
    max-width: 200px;
  }
}
</style>
