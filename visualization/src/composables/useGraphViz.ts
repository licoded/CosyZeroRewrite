/**
 * Composable for GraphViz DOT rendering
 * Uses @viz-js/viz library for synchronous SVG generation
 */

import { ref } from 'vue';
import type { SubStepHighlights } from '@/types/trace';
import { instance } from '@viz-js/viz';

// Viz instance type - use any to avoid complex type issues
let vizModule: any = null;

export interface GraphVizOptions {
  engine?: 'dot' | 'circo' | 'fdp' | 'neato' | 'osage' | 'twopi';
  format?: 'svg' | 'dot' | 'json' | 'xdot' | 'plain' | 'png';
}

/**
 * Composable for GraphViz rendering with highlight support
 */
export function useGraphViz() {
  const isLoading = ref(false);
  const error = ref<string | null>(null);
  const svgContent = ref('');

  /**
   * Initialize the viz.js module (lazy loading)
   */
  async function initViz(): Promise<boolean> {
    if (vizModule) return true;

    try {
      isLoading.value = true;
      vizModule = await instance();
      return true;
    } catch (e) {
      error.value = `Failed to load viz.js: ${e}`;
      console.error(error.value);
      return false;
    } finally {
      isLoading.value = false;
    }
  }

  /**
   * Render a DOT string to SVG
   */
  async function renderDot(dot: string, options: GraphVizOptions = {}): Promise<string> {
    if (!await initViz()) {
      return '';
    }

    try {
      const result = vizModule!.render(dot, {
        format: options.format || 'svg',
        engine: options.engine || 'dot'
      });

      // RenderResult: { status: "success", output: string, errors: [] }
      if (result?.status === 'success') {
        return result.output;
      }
      if (result?.status === 'failure') {
        error.value = `DOT rendering failed: ${result.errors?.map((e: any) => e.message).join(', ')}`;
        return '';
      }
      return '';
    } catch (e) {
      error.value = `Failed to render DOT: ${e}`;
      console.error(error.value);
      return '';
    }
  }

  /**
   * Apply highlights to an SVG element
   * This modifies the SVG DOM directly
   */
  function applyHighlights(
    svgElement: SVGSVGElement,
    highlights: SubStepHighlights
  ): void {
    // Helper to find a node/group by its title
    function findNodeByTitle(nodeId: string): SVGGElement | null {
      const titles = svgElement.querySelectorAll('title');
      for (const title of titles) {
        if (title.textContent && title.textContent.startsWith(nodeId)) {
          const parent = title.parentElement as unknown as SVGGElement | null;
          return parent;
        }
      }
      return null;
    }

    // Helper to find an edge by from/to nodes
    function findEdge(from: string, to: string): SVGElement | null {
      const edges = svgElement.querySelectorAll('.edge');
      for (const edge of edges) {
        const title = edge.querySelector('title');
        if (title && title.textContent) {
          const text = title.textContent;
          // DOT edge title format: "from -> to"
          if (text.includes(`${from} -> ${to}`)) {
            return edge as SVGElement;
          }
        }
      }
      return null;
    }

    // Apply highlight class to nodes
    function highlightNode(nodeId: string, className: string): void {
      const node = findNodeByTitle(nodeId);
      if (node) {
        node.classList.add(className);
      }
    }

    // Apply highlight class to edges
    function highlightEdge(from: string, to: string, className: string): void {
      const edge = findEdge(from, to);
      if (edge) {
        edge.classList.add(className);
      }
    }

    // Apply all highlights
    if (highlights.new_nodes) {
      for (const id of highlights.new_nodes) {
        highlightNode(id, 'highlight-new');
      }
    }

    if (highlights.scc_nodes) {
      for (const id of highlights.scc_nodes) {
        highlightNode(id, 'highlight-scc');
      }
    }

    if (highlights.pending_nodes) {
      for (const id of highlights.pending_nodes) {
        highlightNode(id, 'highlight-pending');
      }
    }

    if (highlights.attractor_nodes) {
      for (const id of highlights.attractor_nodes) {
        highlightNode(id, 'highlight-attractor');
      }
    }

    if (highlights.updated_nodes) {
      for (const id of highlights.updated_nodes) {
        highlightNode(id, 'highlight-updated');
      }
    }

    if (highlights.new_edges) {
      for (const edge of highlights.new_edges) {
        highlightEdge(edge.from, edge.to, 'highlight-new');
      }
    }
  }

  return {
    isLoading,
    error,
    svgContent,
    initViz,
    renderDot,
    applyHighlights
  };
}

/**
 * Parse state ID from DOT node label
 * Extracts "S0" from "S0\nEwin" etc.
 */
export function parseStateId(label: string): string {
  const parts = label.split('\n');
  return parts[0]?.trim() ?? label;
}
