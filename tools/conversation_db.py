#!/usr/bin/env python3
"""
MetricStream Conversation Database Manager
Stores and organizes technical conversations for knowledge management
"""

import sqlite3
import json
import argparse
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Optional, Tuple

class ConversationDB:
    def __init__(self, db_path: str = "metricstream_conversations.db"):
        self.db_path = db_path
        self.init_database()
    
    def init_database(self):
        """Initialize the database with required tables"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # Main conversations table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS conversations (
                id INTEGER PRIMARY KEY,
                session_date DATE,
                topic VARCHAR(255),
                phase VARCHAR(50),
                summary TEXT,
                raw_content TEXT,
                duration_minutes INTEGER,
                participants TEXT, -- JSON array
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # Technical decisions made during conversations
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS technical_decisions (
                id INTEGER PRIMARY KEY,
                conversation_id INTEGER,
                decision_type VARCHAR(100),
                problem_description TEXT,
                solution_chosen TEXT,
                alternatives_considered TEXT,
                rationale TEXT,
                code_files_affected TEXT, -- JSON array
                performance_impact TEXT,
                FOREIGN KEY (conversation_id) REFERENCES conversations(id)
            )
        ''')
        
        # Code changes resulting from conversations
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS code_changes (
                id INTEGER PRIMARY KEY,
                conversation_id INTEGER,
                file_path VARCHAR(500),
                change_type VARCHAR(50), -- 'added', 'modified', 'deleted'
                before_snippet TEXT,
                after_snippet TEXT,
                line_numbers VARCHAR(100),
                purpose TEXT,
                commit_hash VARCHAR(40),
                FOREIGN KEY (conversation_id) REFERENCES conversations(id)
            )
        ''')
        
        # Concepts learned during conversations
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS concepts_learned (
                id INTEGER PRIMARY KEY,
                conversation_id INTEGER,
                concept_name VARCHAR(255),
                description TEXT,
                code_examples TEXT,
                related_files TEXT, -- JSON array
                difficulty_level INTEGER, -- 1-5 scale
                mastery_level INTEGER, -- 1-5 scale (how well understood)
                prerequisites TEXT, -- JSON array of concept names
                FOREIGN KEY (conversation_id) REFERENCES conversations(id)
            )
        ''')
        
        # Cross-references between conversations
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS cross_references (
                id INTEGER PRIMARY KEY,
                from_conversation_id INTEGER,
                to_conversation_id INTEGER,
                relationship_type VARCHAR(100), -- 'builds_on', 'contradicts', 'related', 'prerequisite'
                description TEXT,
                strength REAL, -- 0.0-1.0 relevance score
                FOREIGN KEY (from_conversation_id) REFERENCES conversations(id),
                FOREIGN KEY (to_conversation_id) REFERENCES conversations(id)
            )
        ''')
        
        # Performance metrics tracked during conversations
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS performance_metrics (
                id INTEGER PRIMARY KEY,
                conversation_id INTEGER,
                metric_name VARCHAR(100),
                before_value REAL,
                after_value REAL,
                unit VARCHAR(50),
                measurement_context TEXT,
                FOREIGN KEY (conversation_id) REFERENCES conversations(id)
            )
        ''')
        
        # Create indexes for common queries
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_conversations_topic ON conversations(topic)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_conversations_phase ON conversations(phase)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_conversations_date ON conversations(session_date)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_decisions_type ON technical_decisions(decision_type)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_changes_file ON code_changes(file_path)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_concepts_name ON concepts_learned(concept_name)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_performance_metric ON performance_metrics(metric_name)')
        
        conn.commit()
        conn.close()
    
    def add_conversation(self, session_date: str, topic: str, phase: str, 
                        summary: str, raw_content: str, duration_minutes: int = None,
                        participants: List[str] = None) -> int:
        """Add a new conversation record"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        participants_json = json.dumps(participants or ["human", "claude"])
        
        cursor.execute('''
            INSERT INTO conversations 
            (session_date, topic, phase, summary, raw_content, duration_minutes, participants)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        ''', (session_date, topic, phase, summary, raw_content, duration_minutes, participants_json))
        
        conversation_id = cursor.lastrowid
        conn.commit()
        conn.close()
        return conversation_id
    
    def add_technical_decision(self, conversation_id: int, decision_type: str,
                             problem_description: str, solution_chosen: str,
                             alternatives_considered: str = None, rationale: str = None,
                             code_files_affected: List[str] = None, performance_impact: str = None):
        """Add a technical decision record"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        files_json = json.dumps(code_files_affected or [])
        
        cursor.execute('''
            INSERT INTO technical_decisions 
            (conversation_id, decision_type, problem_description, solution_chosen,
             alternatives_considered, rationale, code_files_affected, performance_impact)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (conversation_id, decision_type, problem_description, solution_chosen,
              alternatives_considered, rationale, files_json, performance_impact))
        
        conn.commit()
        conn.close()
    
    def add_code_change(self, conversation_id: int, file_path: str, change_type: str,
                       before_snippet: str = None, after_snippet: str = None,
                       line_numbers: str = None, purpose: str = None, commit_hash: str = None):
        """Add a code change record"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT INTO code_changes 
            (conversation_id, file_path, change_type, before_snippet, after_snippet,
             line_numbers, purpose, commit_hash)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (conversation_id, file_path, change_type, before_snippet, after_snippet,
              line_numbers, purpose, commit_hash))
        
        conn.commit()
        conn.close()
    
    def add_concept_learned(self, conversation_id: int, concept_name: str, description: str,
                           code_examples: str = None, related_files: List[str] = None,
                           difficulty_level: int = 3, mastery_level: int = 3,
                           prerequisites: List[str] = None):
        """Add a concept learned record"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        files_json = json.dumps(related_files or [])
        prereq_json = json.dumps(prerequisites or [])
        
        cursor.execute('''
            INSERT INTO concepts_learned 
            (conversation_id, concept_name, description, code_examples, related_files,
             difficulty_level, mastery_level, prerequisites)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (conversation_id, concept_name, description, code_examples, files_json,
              difficulty_level, mastery_level, prereq_json))
        
        conn.commit()
        conn.close()
    
    def query_conversations_by_topic(self, topic: str) -> List[Dict]:
        """Find conversations related to a specific topic"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT id, session_date, topic, phase, summary, duration_minutes
            FROM conversations 
            WHERE topic LIKE ? OR summary LIKE ?
            ORDER BY session_date DESC
        ''', (f'%{topic}%', f'%{topic}%'))
        
        results = []
        for row in cursor.fetchall():
            results.append({
                'id': row[0],
                'session_date': row[1],
                'topic': row[2],
                'phase': row[3],
                'summary': row[4],
                'duration_minutes': row[5]
            })
        
        conn.close()
        return results
    
    def get_learning_progression(self, concept: str) -> List[Dict]:
        """Track learning progression for a specific concept"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT c.session_date, cl.concept_name, cl.difficulty_level, 
                   cl.mastery_level, c.phase, c.summary
            FROM conversations c
            JOIN concepts_learned cl ON c.id = cl.conversation_id
            WHERE cl.concept_name LIKE ?
            ORDER BY c.session_date
        ''', (f'%{concept}%',))
        
        results = []
        for row in cursor.fetchall():
            results.append({
                'session_date': row[0],
                'concept_name': row[1],
                'difficulty_level': row[2],
                'mastery_level': row[3],
                'phase': row[4],
                'summary': row[5]
            })
        
        conn.close()
        return results
    
    def get_decision_timeline(self) -> List[Dict]:
        """Get chronological timeline of technical decisions"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT c.session_date, c.phase, td.decision_type, td.problem_description,
                   td.solution_chosen, td.performance_impact
            FROM conversations c
            JOIN technical_decisions td ON c.id = td.conversation_id
            ORDER BY c.session_date
        ''')
        
        results = []
        for row in cursor.fetchall():
            results.append({
                'date': row[0],
                'phase': row[1],
                'decision_type': row[2],
                'problem': row[3],
                'solution': row[4],
                'performance_impact': row[5]
            })
        
        conn.close()
        return results
    
    def export_to_markdown(self, output_dir: str):
        """Export database contents to organized markdown files"""
        output_path = Path(output_dir)
        output_path.mkdir(exist_ok=True)
        
        # Export conversation timeline
        timeline_path = output_path / "conversation_timeline.md"
        with open(timeline_path, 'w') as f:
            f.write("# MetricStream Conversation Timeline\n\n")
            
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()
            
            cursor.execute('''
                SELECT session_date, topic, phase, summary
                FROM conversations
                ORDER BY session_date
            ''')
            
            for row in cursor.fetchall():
                f.write(f"## {row[0]} - {row[1]} ({row[2]})\n")
                f.write(f"{row[3]}\n\n")
            
            conn.close()
        
        # Export technical decisions
        decisions_path = output_path / "technical_decisions.md"
        with open(decisions_path, 'w') as f:
            f.write("# Technical Decisions Log\n\n")
            
            decisions = self.get_decision_timeline()
            for decision in decisions:
                f.write(f"## {decision['date']} - {decision['decision_type']}\n")
                f.write(f"**Phase:** {decision['phase']}\n\n")
                f.write(f"**Problem:** {decision['problem']}\n\n")
                f.write(f"**Solution:** {decision['solution']}\n\n")
                if decision['performance_impact']:
                    f.write(f"**Performance Impact:** {decision['performance_impact']}\n\n")
                f.write("---\n\n")
        
        print(f"Exported conversation data to {output_dir}")

def main():
    parser = argparse.ArgumentParser(description='MetricStream Conversation Database Manager')
    parser.add_argument('--init', action='store_true', help='Initialize database')
    parser.add_argument('--add-sample-data', action='store_true', help='Add sample conversation data')
    parser.add_argument('--query-topic', type=str, help='Search conversations by topic')
    parser.add_argument('--export-md', type=str, help='Export to markdown files in specified directory')
    parser.add_argument('--db-path', type=str, default='metricstream_conversations.db', help='Database file path')
    
    args = parser.parse_args()
    
    db = ConversationDB(args.db_path)
    
    if args.init:
        print(f"Database initialized at {args.db_path}")
    
    if args.add_sample_data:
        # Add sample MetricStream conversation data
        conv_id = db.add_conversation(
            session_date="2024-10-01",
            topic="Phase 4 Mutex Optimization", 
            phase="Phase 4",
            summary="Analyzed double mutex bottleneck in rate limiter, implemented hash-based per-client mutex pool to eliminate serialization",
            raw_content="[Detailed conversation content would go here]",
            duration_minutes=120
        )
        
        db.add_technical_decision(
            conversation_id=conv_id,
            decision_type="performance_optimization",
            problem_description="Double mutex serialization: rate_limiter_mutex_ followed by metrics_mutex_ serializes all client requests",
            solution_chosen="Hash-based pre-allocated mutex pool with per-client locking using std::array<std::mutex, MUTEX_POOL_SIZE>",
            alternatives_considered="Remove mutexes entirely (data corruption risk), std::map<string, mutex> (constructor races)",
            rationale="Eliminates constructor races while enabling true per-client concurrency",
            code_files_affected=["include/ingestion_service.h", "src/ingestion_service.cpp"],
            performance_impact="Expected 50-80% improvement in multi-client scenarios"
        )
        
        db.add_concept_learned(
            conversation_id=conv_id,
            concept_name="Mutex Constructor Race Conditions",
            description="When using std::map<string, mutex>, concurrent access can trigger map rehashing during mutex object construction, leading to undefined behavior",
            difficulty_level=4,
            mastery_level=4,
            prerequisites=["C++ std::map internals", "Memory allocation", "Thread synchronization"]
        )
        
        print("Sample data added successfully")
    
    if args.query_topic:
        results = db.query_conversations_by_topic(args.query_topic)
        print(f"\nConversations about '{args.query_topic}':")
        for conv in results:
            print(f"  {conv['session_date']} - {conv['topic']} ({conv['phase']})")
            print(f"    {conv['summary'][:100]}...")
    
    if args.export_md:
        db.export_to_markdown(args.export_md)

if __name__ == "__main__":
    main()