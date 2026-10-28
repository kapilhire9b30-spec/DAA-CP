import React, { useEffect, useState } from 'react';
import { apiFetch, ApiError } from '../../services/api';
import { useToast } from '../../context/ToastContext';
import { CompareResponse, CompareRow } from '../../types/api';
import { SkeletonCard, SkeletonTable } from '../../components/Skeleton';
import { SparklesIcon } from '../../components/Icons';

export const AlgorithmInsightsPage: React.FC = () => {
  const { addToast } = useToast();

  const [compareData, setCompareData] = useState<CompareResponse | null>(null);
  const [loadingCompare, setLoadingCompare] = useState(true);

  const [demoData, setDemoData] = useState<CompareResponse | null>(null);
  const [loadingDemo, setLoadingDemo] = useState(false);

  const loadCompare = async () => {
    setLoadingCompare(true);
    try {
      const data = await apiFetch<CompareResponse>('/api/compare');
      setCompareData(data);
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to load comparison data', 'error');
      }
    } finally {
      setLoadingCompare(false);
    }
  };

  useEffect(() => {
    loadCompare();
  }, []);

  const runDemoBenchmark = async () => {
    setLoadingDemo(true);
    try {
      const data = await apiFetch<CompareResponse>('/api/demo');
      setDemoData(data);
      addToast('Demo benchmark scenario executed!', 'success');
    } catch (err: unknown) {
      if (err instanceof ApiError) {
        addToast(err.message, 'error');
      } else {
        addToast('Failed to run demo comparison', 'error');
      }
    } finally {
      setLoadingDemo(false);
    }
  };

  return (
    <div className="page-container">
      {/* Header */}
      <div className="page-header">
        <div>
          <h1 className="page-title">Algorithm Insights & Evaluation</h1>
          <p className="page-description">
            Technical benchmarking and comparative analysis tooling for C++ scheduling algorithms.
            (Read-only evaluation tool; does not modify database records).
          </p>
        </div>

        <button type="button" onClick={loadCompare} className="btn btn-secondary btn-sm" disabled={loadingCompare}>
          Refresh Benchmark
        </button>
      </div>

      {/* 1. Compare All Algorithms Section */}
      <div className="card">
        <div className="card-header">
          <div>
            <h2 className="card-title">Live Algorithm Comparison Table</h2>
            <p className="card-subtitle">
              Evaluation of all 3 optimization solvers against currently pending campus event requests
            </p>
          </div>
        </div>

        {loadingCompare ? (
          <SkeletonTable rows={3} />
        ) : !compareData || !compareData.rows ? (
          <p className="text-muted">No comparison data available.</p>
        ) : (
          <div className="table-responsive">
            <table className="data-table">
              <thead>
                <tr>
                  <th>Algorithm</th>
                  <th>Allocated</th>
                  <th>Total Weight</th>
                  <th>Runtime ms</th>
                  <th>Violations</th>
                  <th>Unallocated IDs</th>
                </tr>
              </thead>
              <tbody>
                {compareData.rows.map((row: CompareRow, idx: number) => (
                  <tr key={idx}>
                    <td><strong>{row.algorithm}</strong></td>
                    <td>{row.allocatedCount} requests</td>
                    <td><strong className="text-primary font-mono">{row.totalWeight}</strong></td>
                    <td><span className="font-mono text-muted">{row.runtimeMs.toFixed(3)} ms</span></td>
                    <td>
                      {row.violations.length === 0 ? (
                        <span className="text-success font-medium">0 (None)</span>
                      ) : (
                        <span className="text-danger font-medium">{row.violations.join(', ')}</span>
                      )}
                    </td>
                    <td>
                      {row.unallocated.length === 0 ? (
                        <span className="text-muted">None</span>
                      ) : (
                        row.unallocated.map((id) => `#${id}`).join(', ')
                      )}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}
      </div>

      {/* 2. Greedy vs Optimal Demo Section */}
      <div className="card">
        <div className="card-header">
          <div>
            <h2 className="card-title">Greedy vs Optimal Benchmark Demo</h2>
            <p className="card-subtitle">
              Side-effect free execution of the canonical Test 9 scenario: 3 same-tier requests competing for 1 room
            </p>
          </div>

          <button
            type="button"
            onClick={runDemoBenchmark}
            disabled={loadingDemo}
            className="btn btn-primary"
          >
            <SparklesIcon size={16} />
            {loadingDemo ? 'Running Benchmark...' : 'Run Demo Scenario (GET /api/demo)'}
          </button>
        </div>

        {/* Plain-English Explanation */}
        <div className="alert alert-info mb-4" role="note">
          <h4 style={{ fontWeight: 700, marginBottom: '0.35rem' }}>Why Results Differ (Plain English):</h4>
          <p style={{ fontSize: '0.88rem', lineHeight: '1.6' }}>
            When multiple event requests of equal priority tier overlap in time for a single available room,
            different heuristics make conflicting choices:
          </p>
          <ul style={{ paddingLeft: '1.25rem', marginTop: '0.4rem', fontSize: '0.85rem', lineHeight: '1.6' }}>
            <li>
              <strong>Priority Greedy (Algorithm A):</strong> Chooses the request with the highest attendance first.
              If that large request sits directly in the middle of the time window, it blocks both the earlier
              and later events, resulting in only 1 event scheduled.
            </li>
            <li>
              <strong>Earliest Finish Time (Algorithm B):</strong> Prioritizes the request that finishes earliest,
              leaving maximum remaining room time. It successfully packs 2 compatible non-overlapping events into the room.
            </li>
            <li>
              <strong>Exhaustive Search with Pruning (Algorithm C):</strong> Explores all combinatorial possibilities with
              branch-and-bound pruning to mathematically guarantee the maximum total objective weight.
            </li>
          </ul>
        </div>

        {loadingDemo && <SkeletonCard count={1} />}

        {demoData && demoData.rows && (
          <div className="table-responsive">
            <table className="data-table">
              <thead>
                <tr>
                  <th>Algorithm</th>
                  <th>Allocated</th>
                  <th>Total Weight</th>
                  <th>Runtime ms</th>
                  <th>Violations</th>
                  <th>Assigned Pairs</th>
                </tr>
              </thead>
              <tbody>
                {demoData.rows.map((row: CompareRow, idx: number) => {
                  const maxWeight = Math.max(...demoData.rows.map((r) => r.totalWeight));
                  const isOptimal = row.totalWeight === maxWeight;

                  return (
                    <tr key={idx} style={isOptimal ? { backgroundColor: 'var(--success-bg)' } : undefined}>
                      <td>
                        <strong>{row.algorithm}</strong>
                        {isOptimal && (
                          <span className="badge badge-confirmed ml-2">Guaranteed Optimal</span>
                        )}
                      </td>
                      <td>{row.allocatedCount} requests</td>
                      <td><strong className="text-primary font-mono">{row.totalWeight}</strong></td>
                      <td><span className="font-mono text-muted">{row.runtimeMs.toFixed(3)} ms</span></td>
                      <td>
                        {row.violations.length === 0 ? (
                          <span className="text-success font-medium">None</span>
                        ) : (
                          <span className="text-danger font-medium">{row.violations.join(', ')}</span>
                        )}
                      </td>
                      <td>
                        {row.assignments.length === 0 ? (
                          <span className="text-muted">None</span>
                        ) : (
                          row.assignments.map((a, i) => (
                            <span key={i} className="badge badge-default mr-1">
                              Req #{a.requestId} &rarr; Room #{a.roomId}
                            </span>
                          ))
                        )}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        )}
      </div>
    </div>
  );
};
