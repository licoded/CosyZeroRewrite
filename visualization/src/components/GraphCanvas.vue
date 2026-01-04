<!--
  GraphCanvas.vue
  Component for displaying DOT graphs with SVG rendering and highlights
-->
<template>
  <div class="graph-canvas" ref="containerRef">
    <div v-if="isLoading" class="loading">Loading graph...</div>
    <div v-else-if="error" class="error">{{ error }}</div>
    <div
      v-else
      v-html="svgContent"
      ref="svgRef"
      class="svg-container"
      @mouseover="handleMouseOver"
      @mouseout="handleMouseOut"
    ></div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, nextTick, onMounted } from 'vue';
import { useGraphViz } from '@/composables/useGraphViz';
import type { SubStepHighlights, SubStepGraphData } from '@/types/trace';

interface Props {
  graphData: SubStepGraphData;
  highlights?: SubStepHighlights;
}

const props = withDefaults(defineProps<Props>(), {
  highlights: () => ({} as SubStepHighlights)
});

const emit = defineEmits<{
  nodeHover: [nodeId: string | null];
}>();

const containerRef = ref<HTMLElement>();
const svgRef = ref<HTMLElement>();

const { isLoading, error, svgContent, renderDot, applyHighlights } = useGraphViz();

/**
 * Render the graph when graphData changes
 */
async function render(): Promise<void> {
  if (!props.graphData.dot) return;

  const svg = await renderDot(props.graphData.dot);
  svgContent.value = svg;

  // Wait for DOM update then apply highlights
  await nextTick();
  if (svgRef.value && props.highlights) {
    const svgElement = svgRef.value.querySelector('svg');
    if (svgElement) {
      applyHighlights(svgElement as SVGSVGElement, props.highlights);
    }
  }
}

/**
 * Handle mouse over for node hover detection
 */
function handleMouseOver(event: MouseEvent): void {
  const target = event.target as SVGElement;
  if (!target || !svgRef.value) return;

  // Find the containing group/node
  let node: SVGElement | null = target;
  while (node && node.tagName !== 'g') {
    node = node.parentElement as SVGElement;
    if (node === svgRef.value) {
      node = null;
      break;
    }
  }

  if (!node) return;

  // Find the title element for node ID
  const title = node.querySelector('title');
  if (title && title.textContent) {
    const nodeId = title.textContent.split('\n')[0].trim();
    emit('nodeHover', nodeId);
  }
}

/**
 * Handle mouse out
 */
function handleMouseOut(): void {
  emit('nodeHover', null);
}

// Watch for changes
watch(() => props.graphData, render, { deep: true });
watch(() => props.highlights, async () => {
  await render();
}, { deep: true });

onMounted(() => {
  render();
});
</script>

<style scoped>
.graph-canvas {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: white;
  border-radius: 8px;
  overflow: auto;
  position: relative;
}

.loading, .error {
  padding: 20px;
  text-align: center;
}

.error {
  color: #d32f2f;
}

.svg-container {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
}

/* Highlight styles applied via JS */
:deep(.highlight-new) {
  filter: drop-shadow(0 0 8px #2196F3);
}

:deep(.highlight-new) path, :deep(.highlight-new) ellipse, :deep(.highlight-new) polygon {
  stroke: #2196F3;
  stroke-width: 3;
}

:deep(.highlight-scc) {
  filter: drop-shadow(0 0 6px #FFD700);
}

:deep(.highlight-scc) path, :deep(.highlight-scc) ellipse, :deep(.highlight-scc) polygon {
  stroke: #FFD700;
  stroke-width: 3;
}

:deep(.highlight-pending) {
  opacity: 0.5;
}

:deep(.highlight-updated) {
  filter: drop-shadow(0 0 6px #FF5722);
}

:deep(.highlight-updated) path, :deep(.highlight-updated) ellipse, :deep(.highlight-updated) polygon {
  stroke: #FF5722;
  stroke-width: 3;
}

:deep(.highlight-attractor) {
  filter: drop-shadow(0 0 6px #9C27B0);
}

:deep(.highlight-attractor) path, :deep(.highlight-attractor) ellipse, :deep(.highlight-attractor) polygon {
  stroke: #9C27B0;
  stroke-width: 3;
}
</style>
