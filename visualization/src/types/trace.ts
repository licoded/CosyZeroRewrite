/**
 * Type definitions for LTLf Synthesis Trace Data
 * Corresponds to the JSON schema in docs/TRACE_VISUALIZATION/json_schema.md
 */

export interface TraceEdge {
  from: string;
  to: string;
  label?: string;
  type?: 'sys_move' | 'env_move';
}

export interface SubStepHighlights {
  new_nodes?: string[];
  new_edges?: TraceEdge[];
  updated_nodes?: string[];
  scc_nodes?: string[];
  scc_id?: string;
  pending_nodes?: string[];
  attractor_nodes?: string[];
}

export interface SubStepGraphData {
  dot: string;
  num_nodes: number;
  num_edges: number;
}

export interface SubStepStateInfo {
  swin_count: number;
  ewin_count: number;
  unknown_count: number;
  total_states: number;
  current_scc?: number;
}

export interface SubStepMetrics {
  duration_ms: number;
  memory_kb?: number;
}

export interface SubStep {
  step_id: string;
  description?: string;
  graph_data: SubStepGraphData;
  highlights: SubStepHighlights;
  state_info: SubStepStateInfo;
  metrics: SubStepMetrics;
}

export type StageType = 'expand' | 'scc' | 'fixed_point' | 'attractor';

export interface TraceStage {
  stage_id: string;
  stage_type: StageType;
  description: string;
  sub_steps: SubStep[];
}

export interface TraceSummary {
  total_steps: number;
  total_states: number;
  total_sccs?: number;
  realizable: boolean;
  duration_ms: number;
  stages_summary?: Array<{ stage_type: string; steps_count: number }>;
}

export interface Trace {
  formula: string;
  timestamp: string;
  formula_id?: string;
  stages: TraceStage[];
  summary?: TraceSummary;
}

// State information for tooltip display
export interface StateData {
  id: string;
  classification: 'Swin' | 'Ewin' | 'Unknown';
  type: 'System' | 'Environment';
  is_initial: boolean;
  phi?: string;
  xnf_phi?: string;
  prop_atoms?: string[];
}

export type StateClass = 'Unknown' | 'Swin' | 'Ewin' | 'Draw';
