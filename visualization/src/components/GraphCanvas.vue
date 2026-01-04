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
      :class="{ 'edge-tooltip': tooltip.isEdge }"
      :style="{ left: tooltip.x + 'px', top: tooltip.y + 'px' }"
    >
      <!-- Node Tooltip -->
      <template v-if="!tooltip.isEdge">
        <div class="tooltip-header">
          {{ tooltip.stateData?.id || '' }}
          <span v-if="tooltip.stateData?.is_initial" class="initial-badge">(initial)</span>
        </div>
        <div class="tooltip-row">
          <span class="tooltip-label">Classification:</span>
          <span
            v-if="tooltip.stateData"
            :class="['classification-badge', tooltip.stateData.classification?.toLowerCase()]"
          >
            {{ tooltip.stateData.classification }}
          </span>
        </div>
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
      </template>

      <!-- Edge Tooltip -->
      <template v-else>
        <div class="tooltip-header">{{ tooltip.edgeInfo?.from }} → {{ tooltip.edgeInfo?.to }}</div>
        <div class="tooltip-row">
          <span class="tooltip-label">Type:</span>
          <span class="tooltip-value">{{ tooltip.edgeInfo?.type || '' }}</span>
        </div>
        <div class="tooltip-row">
          <span class="tooltip-label">Assignment:</span>
          <span class="tooltip-value formula-text">
            {{ tooltip.edgeInfo?.label ? formatAssignmentWithNames(tooltip.edgeInfo.label) : '(none)' }}
          </span>
        </div>
        <div v-if="tooltip.edgeInfo?.formula" class="tooltip-row">
          <span class="tooltip-label">Formula:</span>
          <span class="tooltip-value formula-text">{{ tooltip.edgeInfo.formula }}</span>
        </div>
      </template>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, nextTick, onMounted } from 'vue';
import { useGraphViz } from '@/composables/useGraphViz';
import type { SubStepHighlights, SubStepGraphData, StateData, TracePartition } from '@/types/trace';

interface Props {
  graphData: SubStepGraphData;
  highlights?: SubStepHighlights;
  partition?: TracePartition;
}

const props = withDefaults(defineProps<Props>(), {
  highlights: () => ({} as SubStepHighlights),
  partition: undefined
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
  isEdge: false,
  stateData: null as StateData | null,
  edgeInfo: null as { from: string; to: string; type: string; label: string; formula?: string } | null
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
 * Parse node ID from the title element
 */
function extractNodeId(titleText: string): string {
  const lines = titleText.split('\n').map(l => l.trim());
  return lines[0] || '';
}

/**
 * Parse edge information from the title element
 * Title format: "from -> to"
 * @param titleText The title element text (e.g., "S0 -> E0")
 * @param labelText The text element content (e.g., "sys={p}" or "env={}")
 */
function parseEdgeInfo(titleText: string, labelText: string = ''): { from: string; to: string; type: string; label: string; formula?: string } | null {
  // DOT edge title format: "S0 -> E0"
  const match = titleText.match(/^(\w+)\s*->\s*(\w+)/);
  if (!match) return null;

  const from = match[1];
  const to = match[2];

  // Use the provided labelText (from SVG text element)
  // Trim whitespace that might be in the text element
  const rawLabel = labelText.trim();

  // Determine edge type from the label prefix (sys=/env=)
  let type = 'Unknown move';
  let isSysMove = from.startsWith('S');

  // Check label prefix to determine type
  if (rawLabel.startsWith('sys=')) {
    type = 'System move';
    isSysMove = true;
  } else if (rawLabel.startsWith('env=')) {
    type = 'Environment move';
    isSysMove = false;
  } else {
    // Fallback: determine from node type
    type = isSysMove ? 'System move' : 'Environment move';
  }

  // Convert assignment to formula if partition is available
  const formula = rawLabel ? assignmentToFormula(rawLabel) : undefined;

  return { from, to, type, label: rawLabel, formula };
}

/**
 * Parse assignment string like "sys={p}" or "env={p, q}"
 * Returns the array of variable names (e.g., ["p"] or ["p", "q"])
 */
function parseAssignment(label: string): string[] | null {
  // Match pattern: "sys={...}" or "env={...}" or just "{...}"
  const match = label.match(/\{([^}]*)\}/);
  if (!match) return null;

  const namesStr = match[1];
  if (namesStr.trim() === '') return [];

  // Parse comma-separated variable names
  return namesStr.split(',').map(s => s.trim()).filter(s => s.length > 0);
}

/**
 * Convert assignment label to formula string
 * E.g., "sys={p}" -> "p"
 * E.g., "sys={p,q}" -> "p & q"
 * E.g., "sys={}" -> "true"
 */
function assignmentToFormula(label: string): string | null {
  const names = parseAssignment(label);
  if (names === null) return null;

  if (names.length === 0) return 'true';
  if (names.length === 1) return names[0];
  return names.join(' & ');
}

/**
 * Format assignment label (now just returns the label as-is since
 * the backend already outputs variable names)
 * E.g., "sys={p}" -> "sys={p}"
 * E.g., "env={}" -> "env={}"
 */
function formatAssignmentWithNames(label: string): string {
  // Backend already formats with variable names, just return as-is
  return label;
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

  // Find the containing group/node/edge
  let node: SVGElement | HTMLElement | null = target;
  let depth = 0;
  while (node && depth < 5) {
    const tagName = node.tagName.toLowerCase();

    // Check for edge (class="edge" or class="link")
    if (tagName === 'g' || tagName === 'a' || tagName === 'path') {
      const className = (node as SVGElement).getAttribute('class') || '';
      if (className.includes('edge') || className.includes('link')) {
        // This is an edge - get edge info
        const title = node.querySelector('title');
        const textLabel = node.querySelector('text');

        // Build edge info: from title for nodes, from text for label
        const titleText = title?.textContent || '';
        const labelText = textLabel?.textContent || '';

        // Parse from -> to from title
        const edgeInfo = parseEdgeInfo(titleText, labelText);
        if (edgeInfo) {
          showEdgeTooltip(event.clientX, event.clientY, edgeInfo);
          return;
        }
      }
    }

    if (tagName === 'g') {
      // Check if this group has a title element (indicates it's a node)
      const title = node.querySelector('title');
      if (title?.textContent) {
        const titleText = title.textContent.trim();
        // Skip SVG root title (e.g., "GameGraph")
        if (titleText === 'GameGraph' || titleText.includes('digraph')) {
          node = node.parentElement;
          depth++;
          continue;
        }
        // Check if it's an edge title (contains ->)
        if (titleText.includes('->')) {
          // This is an edge, also look for text label
          const textLabel = node.querySelector('text')?.textContent || '';
          const edgeInfo = parseEdgeInfo(titleText, textLabel);
          if (edgeInfo) {
            showEdgeTooltip(event.clientX, event.clientY, edgeInfo);
          }
        } else {
          // It's a node
          const nodeId = extractNodeId(titleText);
          const stateData = props.graphData.state_data?.[nodeId];
          if (stateData) {
            showTooltip(event.clientX, event.clientY, stateData);
          } else {
            // Fallback: show basic info if state_data not available
            showTooltipBasic(event.clientX, event.clientY, nodeId);
          }
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
 * Show node tooltip with state data
 */
function showTooltip(x: number, y: number, stateData: StateData): void {
  const offsetX = 15;
  const offsetY = 15;
  const posX = Math.min(x + offsetX, window.innerWidth - 520);
  const posY = Math.min(y + offsetY, window.innerHeight - 200);

  tooltip.value = {
    visible: true,
    x: posX,
    y: posY,
    isEdge: false,
    stateData,
    edgeInfo: null
  };
}

/**
 * Show basic node tooltip (fallback when state_data is not available)
 */
function showTooltipBasic(x: number, y: number, nodeId: string): void {
  const type: 'System' | 'Environment' = nodeId.startsWith('S') ? 'System' : 'Environment';
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
 * Show edge tooltip
 */
function showEdgeTooltip(x: number, y: number, edgeInfo: { from: string; to: string; type: string; label: string; formula?: string }): void {
  const offsetX = 15;
  const offsetY = 15;
  const posX = Math.min(x + offsetX, window.innerWidth - 400);
  const posY = Math.min(y + offsetY, window.innerHeight - 150);

  tooltip.value = {
    visible: true,
    x: posX,
    y: posY,
    isEdge: true,
    stateData: null,
    edgeInfo
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
  margin-top: 6px;
  display: flex;
  flex-direction: row;
  align-items: baseline;
  gap: 6px;
}

.tooltip-label {
  color: #aaa;
  font-size: 11px;
  white-space: nowrap;
}

.tooltip-value {
  font-family: monospace;
  word-break: break-word;
}

.formula-text {
  color: #64B5F6;
  max-width: 360px;
  word-wrap: break-word;
  overflow-wrap: break-word;
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
