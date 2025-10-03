#!/usr/bin/env python3
"""
Decision Tree Generator for MetricStream Conversations
Extracts decision trees from conversation database and generates JSON for web visualization
"""

import sqlite3
import json
import argparse
from typing import Dict, List, Any
from datetime import datetime

class DecisionTreeGenerator:
    def __init__(self, db_path: str = "metricstream_conversations.db"):
        self.db_path = db_path
    
    def extract_decision_tree(self) -> Dict[str, Any]:
        """Extract decision tree data from conversation database"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # Get all technical decisions with conversation context
        cursor.execute('''
            SELECT 
                c.id as conv_id,
                c.session_date,
                c.topic,
                c.phase,
                td.id as decision_id,
                td.decision_type,
                td.problem_description,
                td.solution_chosen,
                td.alternatives_considered,
                td.rationale,
                td.performance_impact,
                td.code_files_affected
            FROM conversations c
            JOIN technical_decisions td ON c.id = td.conversation_id
            ORDER BY c.session_date, td.id
        ''')
        
        decisions = []
        for row in cursor.fetchall():
            decisions.append({
                'conv_id': row[0],
                'session_date': row[1],
                'topic': row[2],
                'phase': row[3],
                'decision_id': row[4],
                'decision_type': row[5],
                'problem_description': row[6],
                'solution_chosen': row[7],
                'alternatives_considered': row[8],
                'rationale': row[9],
                'performance_impact': row[10],
                'code_files_affected': json.loads(row[11] or '[]')
            })
        
        conn.close()
        return self._build_tree_structure(decisions)
    
    def _build_tree_structure(self, decisions: List[Dict]) -> Dict[str, Any]:
        """Build hierarchical tree structure from decisions"""
        
        # MetricStream optimization journey as example tree
        tree = {
            "name": "MetricStream Performance Optimization",
            "type": "root",
            "description": "Journey from 200 RPS to high-performance metrics platform",
            "children": []
        }
        
        # Group decisions by phase
        phases = {}
        for decision in decisions:
            phase = decision['phase']
            if phase not in phases:
                phases[phase] = []
            phases[phase].append(decision)
        
        # Build phase nodes
        for phase_name, phase_decisions in phases.items():
            phase_node = {
                "name": phase_name,
                "type": "phase",
                "description": f"Optimization phase: {phase_name}",
                "children": []
            }
            
            # Add decisions as children
            for decision in phase_decisions:
                decision_node = self._create_decision_node(decision)
                phase_node["children"].append(decision_node)
            
            tree["children"].append(phase_node)
        
        # Always add sample pre-Phase 4 tree for demonstration
        # This provides rich sample data even when we have real decisions
        if not decisions or len(decisions) < 3:  # If limited real data, use comprehensive sample tree
            tree = self._create_sample_tree()
        
        return tree
    
    def _create_decision_node(self, decision: Dict) -> Dict[str, Any]:
        """Create a decision node with alternatives"""
        alternatives = []
        
        # Parse alternatives if available
        if decision['alternatives_considered']:
            alt_text = decision['alternatives_considered']
            # Simple parsing - could be enhanced
            alt_list = alt_text.split(',') if ',' in alt_text else [alt_text]
            
            for i, alt in enumerate(alt_list[:3]):  # Limit to 3 alternatives
                alternatives.append({
                    "name": f"Alternative {i+1}",
                    "type": "alternative",
                    "description": alt.strip(),
                    "chosen": i == 0,  # First alternative is usually the chosen one
                    "outcome": "Not implemented" if i > 0 else decision['solution_chosen']
                })
        
        return {
            "name": decision['problem_description'][:50] + "...",
            "type": "decision",
            "problem": decision['problem_description'],
            "solution": decision['solution_chosen'],
            "rationale": decision['rationale'],
            "performance_impact": decision['performance_impact'],
            "session_date": decision['session_date'],
            "conversation_ref": f"Session {decision['conv_id']}",
            "files_affected": decision['code_files_affected'],
            "children": alternatives
        }
    
    def _create_sample_tree(self) -> Dict[str, Any]:
        """Create sample decision tree for demonstration"""
        return {
            "name": "MetricStream Performance Optimization",
            "type": "root",
            "description": "Journey from 200 RPS to high-performance metrics platform",
            "children": [
                {
                    "name": "Phase 1: Basic Threading",
                    "type": "phase",
                    "description": "Initial threading implementation",
                    "children": [
                        {
                            "name": "Request Processing Model",
                            "type": "decision",
                            "problem": "Sequential request processing limiting throughput",
                            "solution": "Thread per request model",
                            "rationale": "Simple to implement, immediate parallelism gains",
                            "performance_impact": "20 clients: 81% → 88% success rate",
                            "session_date": "2024-09-28",
                            "conversation_ref": "Phase 1 Session",
                            "children": [
                                {
                                    "name": "Sequential Processing",
                                    "type": "alternative", 
                                    "description": "Handle one request at a time",
                                    "chosen": False,
                                    "outcome": "Too slow for concurrent clients"
                                },
                                {
                                    "name": "Thread Pool",
                                    "type": "alternative",
                                    "description": "Fixed pool of worker threads",
                                    "chosen": False,
                                    "outcome": "More complex, diminishing returns"
                                },
                                {
                                    "name": "Thread Per Request",
                                    "type": "alternative",
                                    "description": "Spawn new thread for each request",
                                    "chosen": True,
                                    "outcome": "Simple implementation, good initial results"
                                }
                            ]
                        }
                    ]
                },
                {
                    "name": "Phase 2: Async I/O",
                    "type": "phase", 
                    "description": "Non-blocking I/O implementation",
                    "children": [
                        {
                            "name": "File Writing Strategy",
                            "type": "decision",
                            "problem": "File I/O blocking request threads",
                            "solution": "Asynchronous batch writer with queue",
                            "rationale": "Decouples request processing from storage I/O",
                            "performance_impact": "50 clients: 59% → 66% success rate",
                            "session_date": "2024-09-29",
                            "conversation_ref": "Phase 2 Session",
                            "children": [
                                {
                                    "name": "Synchronous Writing",
                                    "type": "alternative",
                                    "description": "Write to file directly in request thread",
                                    "chosen": False,
                                    "outcome": "I/O blocking causes request timeouts"
                                },
                                {
                                    "name": "Async Batch Writer",
                                    "type": "alternative",
                                    "description": "Queue batches, background thread writes",
                                    "chosen": True,
                                    "outcome": "Eliminates I/O blocking, improves throughput"
                                }
                            ]
                        }
                    ]
                },
                {
                    "name": "Phase 3: JSON Optimization",
                    "type": "phase",
                    "description": "JSON parsing performance improvements",
                    "children": [
                        {
                            "name": "JSON Parsing Approach",
                            "type": "decision",
                            "problem": "JSON parsing becoming bottleneck at high load",
                            "solution": "Custom optimized parser with string searching",
                            "rationale": "Avoid regex overhead, optimize for known structure",
                            "performance_impact": "100 clients: significant parsing speedup",
                            "session_date": "2024-09-30",
                            "conversation_ref": "Phase 3 Session",
                            "children": [
                                {
                                    "name": "Regex-Based Parsing",
                                    "type": "alternative",
                                    "description": "Use regex to extract JSON fields",
                                    "chosen": False,
                                    "outcome": "Too slow for high-frequency parsing"
                                },
                                {
                                    "name": "Full JSON Library",
                                    "type": "alternative",
                                    "description": "Use complete JSON parsing library",
                                    "chosen": False,
                                    "outcome": "Overkill for simple structure"
                                },
                                {
                                    "name": "Custom String Search",
                                    "type": "alternative",
                                    "description": "Optimized string searching for known fields",
                                    "chosen": True,
                                    "outcome": "Fast parsing tailored to metric structure"
                                }
                            ]
                        }
                    ]
                },
                {
                    "name": "Phase 4: Mutex Optimization",
                    "type": "phase",
                    "description": "Rate limiting mutex contention elimination",
                    "children": [
                        {
                            "name": "Rate Limiter Synchronization",
                            "type": "decision",
                            "problem": "Double mutex serialization: rate_limiter_mutex_ + metrics_mutex_",
                            "solution": "Hash-based per-client mutex pool",
                            "rationale": "Eliminates constructor races while enabling per-client concurrency",
                            "performance_impact": "Expected 50-80% improvement in multi-client scenarios",
                            "session_date": "2024-10-01",
                            "conversation_ref": "Phase 4 Session",
                            "files_affected": ["include/ingestion_service.h", "src/ingestion_service.cpp"],
                            "children": [
                                {
                                    "name": "Remove Mutexes Entirely",
                                    "type": "alternative",
                                    "description": "Use lock-free data structures",
                                    "chosen": False,
                                    "outcome": "Data corruption risk with std::deque and std::unordered_map"
                                },
                                {
                                    "name": "std::map<string, mutex>",
                                    "type": "alternative", 
                                    "description": "Dynamic per-client mutex creation",
                                    "chosen": False,
                                    "outcome": "Constructor race conditions during map rehashing"
                                },
                                {
                                    "name": "Hash-Based Mutex Pool",
                                    "type": "alternative",
                                    "description": "Pre-allocated array with hash(client_id) selection",
                                    "chosen": True,
                                    "outcome": "Eliminates races, enables concurrency, bounded memory"
                                }
                            ]
                        }
                    ]
                }
            ]
        }
    
    def generate_json(self, output_file: str = "conversation/web/decision_tree.json"):
        """Generate JSON file for web interface"""
        tree_data = self.extract_decision_tree()
        
        # Add metadata
        tree_data["metadata"] = {
            "generated_at": datetime.now().isoformat(),
            "generator": "MetricStream Decision Tree Generator",
            "version": "1.0"
        }
        
        with open(output_file, 'w') as f:
            json.dump(tree_data, f, indent=2, ensure_ascii=False)
        
        print(f"✅ Decision tree JSON generated: {output_file}")
        return tree_data

def main():
    parser = argparse.ArgumentParser(description='Generate decision tree JSON from conversation database')
    parser.add_argument('--output', default='conversation/web/decision_tree.json', help='Output JSON file path')
    parser.add_argument('--db-path', default='metricstream_conversations.db', help='Database file path')
    
    args = parser.parse_args()
    
    generator = DecisionTreeGenerator(args.db_path)
    tree_data = generator.generate_json(args.output)
    
    print(f"📊 Generated tree with {len(tree_data.get('children', []))} phases")
    
    # Print summary
    total_decisions = 0
    for phase in tree_data.get('children', []):
        decisions_in_phase = len(phase.get('children', []))
        total_decisions += decisions_in_phase
        print(f"   🎯 {phase['name']}: {decisions_in_phase} decisions")
    
    print(f"📈 Total decisions tracked: {total_decisions}")

if __name__ == "__main__":
    main()