#!/usr/bin/env python3
"""
Quick conversation capture helper
Usage: python3 tools/quick_capture.py "session topic" "key outcome" "main insight"
"""

import sys
import argparse
from datetime import datetime
from pathlib import Path
from conversation_db import ConversationDB

def quick_capture(topic, outcome, insight=None, phase=None):
    """Quickly capture a conversation with minimal input"""
    
    # Generate session date
    session_date = datetime.now().strftime("%Y-%m-%d")
    
    # Create summary
    summary = f"Session outcome: {outcome}"
    if insight:
        summary += f"\n\nKey insight: {insight}"
    
    # Initialize database
    db = ConversationDB()
    
    # Add conversation
    conv_id = db.add_conversation(
        session_date=session_date,
        topic=topic,
        phase=phase or "Unknown",
        summary=summary,
        raw_content="[Captured via quick_capture tool]",
        duration_minutes=None
    )
    
    # Create session file
    session_filename = f"{session_date}_{topic.lower().replace(' ', '_')}.md"
    session_path = Path("conversation/sessions") / session_filename
    
    session_content = f"""# Session: {topic}
**Date:** {session_date}  
**Phase:** {phase or "Unknown"}  
**Participants:** Human, Claude  

## Session Summary

{summary}

## Key Outcomes

- {outcome}
"""
    
    if insight:
        session_content += f"""
## Key Insights

- {insight}
"""
    
    session_content += """
## Technical Details

[Details would be filled in during full conversation capture]

## Follow-Up Items

[To be added as needed]

---
*Created via quick_capture tool - expand with full details as needed*
"""
    
    # Write session file
    session_path.parent.mkdir(exist_ok=True)
    with open(session_path, 'w') as f:
        f.write(session_content)
    
    print(f"✅ Quick capture completed:")
    print(f"   📄 Session file: {session_path}")
    print(f"   📊 Database ID: {conv_id}")
    print(f"   💡 Summary: {outcome}")
    if insight:
        print(f"   🎯 Insight: {insight}")
    print(f"\n🔄 To expand with full details, edit: {session_path}")

def main():
    parser = argparse.ArgumentParser(description='Quick conversation capture')
    parser.add_argument('topic', help='Session topic (e.g., "Phase 5 Optimization")')
    parser.add_argument('outcome', help='Key outcome (e.g., "Reduced latency by 40%")')
    parser.add_argument('--insight', help='Main insight learned')
    parser.add_argument('--phase', help='Development phase')
    
    args = parser.parse_args()
    
    quick_capture(args.topic, args.outcome, args.insight, args.phase)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 tools/quick_capture.py 'Session Topic' 'Key Outcome' [--insight 'Main Insight']")
        print("Example: python3 tools/quick_capture.py 'Phase 5 Optimization' 'Improved throughput by 60%' --insight 'Lock-free queues reduce contention'")
        sys.exit(1)
    
    main()