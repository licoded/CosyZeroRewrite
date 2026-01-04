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
      <div class="tooltip-header">
        {{ tooltip.stateData?.id || '' }}
        <span v-if="tooltip.stateData?.is_initial" class="initial-badge">(initial)</span>
      </div>
      <span
        :class="['classification-badge', tooltip.stateData?.classification?.toLowerCase()]"
      >
        {{ tooltip.stateData?.classification || 'Unknown' }}
      </span>
      <div class="tooltip-row">
        <span class="tooltip-label">Type:</span>
        <span class="tooltip-value">{{ tooltip.stateData?.type || '' }}</span>
      </div>
      <div v-if="tooltip.stateData?.phi && tooltip.stateData.phi !== 'null'" class="tooltip-row">
        <span class="tooltip-label">Formula (phi):</span>
        <span class="tooltip-value formula-text">{{ tooltip.stateData.phi }}</span>
      </div>
      <div v-if="tooltip.stateData?.xnf_phi && tooltip.stateData.xnf_phi !== 'null'" class="tooltip-row">
        <span class="tooltip-label">XNF Formula:</span>
        <span class="tooltip-value formula-text">{{ tooltip.stateData.xnf_phi }}</span>
      </div>
      <div v-if="tooltip.stateData?.prop_atoms && tooltip.stateData.prop_atoms.length > 0" class="tooltip-row">
        <span class="tooltip-label">Propositional Atoms:</span>
        <span class="tooltip-value">[{{ tooltip.stateData.prop_atoms.join(', ') }}]</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, nextTick, onMounted } from 'vue';
import { useGraphViz } from '@/composables/useGraphViz';
import type { SubStepHighlights, SubStepGraphData, StateData } from '@/types/trace';

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
  stateData: null as StateData | null
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
 * Extract node ID from the title element
 * Title format: "S0\nSwin" or "E0\nEwin" or "S0 (init)\nUnknown"
 */
function extractNodeId(titleText: string): string {
  const lines = titleText.split('\n').map(l => l.trim());
  return lines[0] || '';
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
        const nodeId = extractNodeId(title.textContent);
        // Get state data from graphData
        const stateData = props.graphData.state_data?.[nodeId];
        if (stateData) {
          showTooltip(event.clientX, event.clientY, stateData);
        } else {
          // Fallback: show basic info if state_data not available
          showTooltipBasic(event.clientX, event.clientY, nodeId);
        }
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
 * Show detailed tooltip with state data
 */
function showTooltip(x: number, y: number, stateData: StateData): void {
  // Offset tooltip slightly from cursor
  const offsetX = 15;
  const offsetY = 15;

  // Position tooltip, keeping it within viewport
  const posX = Math.min(x + offsetX, window.innerWidth - 520);
  const posY = Math.min(y + offsetY, window.innerHeight - 200);

  tooltip.value = {
    visible: true,
    x: posX,
    y: posY,
    stateData
  };
}

/**
 * Show basic tooltip (fallback when state_data is not available)
 */
function showTooltipBasic(x: number, y: number, nodeId: string): void {
  // Determine player type from node ID prefix
  const type: 'System' | 'Environment' = nodeId.startsWith('S') ? 'System' : 'Environment';

  // Create minimal state data
  const stateData: StateData = {
    id: nodeId,
    classification: 'Unknown',
    type: type,
    is_initial: false,
    phi: '',
    xnf_phi: '',
    prop_atoms: []
  };

  showTooltip(x, y, stateData);
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

/* Tooltip - matches game_graph HTML style */
.node-tooltip {
  position: fixed;
  background: rgba(0, 0, 0, 0.95);
  color: white;
  padding: 12px 16px;
  border-radius: 6px;
  font-size: 13px;
  pointer-events: none;
  z-index: 1000;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
  max-width: 500px;
}

.tooltip-header {
  font-weight: 600;
  font-size: 14px;
  margin-bottom: 8px;
  padding-bottom: 5px;
  border-bottom: 1px solid #444;
  display: flex;
  align-items: center;
  gap: 6px;
}

.initial-badge {
  font-size: 12px;
  color: #FFD700;
}

.classification-badge {
  display: inline-block;
  padding: 2px 8px;
  border-radius: 4px;
  font-size: 12px;
  font-weight: bold;
  margin-bottom: 8px;
}

.classification-badge.swin {
  background: #228b22;
}

.classification-badge.ewin {
  background: #cd5c5c;
}

.classification-badge.unknown,
.classification-badge.draw {
  background: #888;
}

.tooltip-row {
  margin-top: 8px;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.tooltip-label {
  color: #aaa;
  font-size: 11px;
}

.tooltip-value {
  font-family: monospace;
  word-break: break-all;
}

.formula-text {
  color: #64B5F6;
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
