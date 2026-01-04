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
      @mousemove="handleMouseMove"
      @mouseout="handleMouseOut"
    ></div>

    <!-- Floating Tooltip -->
    <div
      v-if="tooltip.visible"
      class="node-tooltip"
      :style="{ left: tooltip.x + 'px', top: tooltip.y + 'px' }"
    >
      <div class="tooltip-header">{{ tooltip.nodeId }}</div>
      <div class="tooltip-info">{{ tooltip.info }}</div>
    </div>
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

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const containerRef = ref<HTMLElement>();
const svgRef = ref<HTMLElement>();

const { isLoading, error, svgContent, renderDot, applyHighlights } = useGraphViz();

// Tooltip state
const tooltip = ref({
  visible: false,
  x: 0,
  y: 0,
  nodeId: '',
  info: ''
});

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
 * Parse node information from the title element
 * Title format: "S0\nSwin" or "E0\nEwin" or "S0 (init)\nUnknown"
 */
function parseNodeInfo(titleText: string): { id: string; info: string } {
  const lines = titleText.split('\n').map(l => l.trim());
  const nodeId = lines[0] || '';

  // Get the classification line (second line, or first line if only one)
  let info = '';
  for (const line of lines) {
    if (line.includes('Swin') || line.includes('Ewin') || line.includes('Unknown')) {
      info = line;
      break;
    }
  }

  // Determine player type from node ID prefix
  const player = nodeId.startsWith('S') ? 'System' : nodeId.startsWith('E') ? 'Environment' : '';

  return {
    id: nodeId,
    info: info ? `${info} · ${player}` : player
  };
}

/**
 * Handle mouse move for tooltip positioning
 */
function handleMouseMove(event: MouseEvent): void {
  const target = event.target as SVGElement;
  if (!target || !svgRef.value) {
    hideTooltip();
    return;
  }

  // Find the containing group/node
  let node: SVGElement | HTMLElement | null = target;
  let depth = 0;
  while (node && depth < 5) {
    if (node.tagName === 'g') {
      // Check if this group has a title element (indicates it's a node)
      const title = node.querySelector('title');
      if (title?.textContent) {
        const nodeInfo = parseNodeInfo(title.textContent);
        showTooltip(event.clientX, event.clientY, nodeInfo.id, nodeInfo.info);
        return;
      }
    }
    node = node.parentElement;
    depth++;
  }

  hideTooltip();
}

/**
 * Handle mouse out
 */
function handleMouseOut(): void {
  hideTooltip();
}

/**
 * Show tooltip at position
 */
function showTooltip(x: number, y: number, nodeId: string, info: string): void {
  // Offset tooltip slightly from cursor
  const offsetX = 15;
  const offsetY = 15;

  tooltip.value = {
    visible: true,
    x: x + offsetX,
    y: y + offsetY,
    nodeId,
    info
  };
}

/**
 * Hide tooltip
 */
function hideTooltip(): void {
  tooltip.value.visible = false;
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
  overflow: hidden;
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

/* Tooltip */
.node-tooltip {
  position: fixed;
  background: rgba(40, 44, 52, 0.95);
  color: white;
  padding: 10px 14px;
  border-radius: 6px;
  font-size: 13px;
  pointer-events: none;
  z-index: 1000;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
  backdrop-filter: blur(4px);
  max-width: 200px;
}

.tooltip-header {
  font-weight: 600;
  font-size: 14px;
  margin-bottom: 4px;
  color: #64B5F6;
}

.tooltip-info {
  color: #c9d1d9;
  font-size: 12px;
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
