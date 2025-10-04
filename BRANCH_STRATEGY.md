# Learning Branch Strategy
## How to Organize Code for the Learning Hub

This document outlines strategies for preserving different states of the codebase to support progressive learning.

---

## Problem Statement

The current `main` branch contains all optimizations (Phases 1-6) already implemented. Learners need starting points with:
- TODOs to implement
- Working baseline code
- Progressive complexity

**Challenge**: Git history shows only 1 commit with final state

---

## Recommended Strategy: Progressive Learning Branches

### Option A: Phase-Based Branches ⭐ (RECOMMENDED)

Create branches representing the state **before** each optimization phase.

#### Branch Structure

```
learning/phase-0-baseline          # Minimal working HTTP server
learning/phase-1-threading         # After Phase 1, before Phase 2
learning/phase-2-async-io          # After Phase 2, before Phase 3
learning/phase-3-json-parsing      # After Phase 3, before Phase 4
learning/phase-4-mutex-pool        # After Phase 4, before Phase 5
learning/phase-5-lock-free         # After Phase 5, before Phase 6
learning/phase-6-thread-pool       # After Phase 6 (current main)
main                               # Latest code with all optimizations
```

#### Implementation Plan

**Step 1: Create Baseline Branch** (learning/phase-0-baseline)
```bash
git checkout -b learning/phase-0-baseline

# Remove ALL optimizations, keep only:
# - Basic HTTP server (blocking, single-threaded)
# - Simple request handling
# - No rate limiting
# - Synchronous file I/O
# - Naive JSON parser

git commit -m "Learning baseline: minimal HTTP server"
git push origin learning/phase-0-baseline
```

**Step 2: Create Phase 1 Branch** (learning/phase-1-threading)
```bash
git checkout -b learning/phase-1-threading learning/phase-0-baseline

# Add Phase 1: Thread-per-request
# Keep Phase 2-6 as TODOs

git commit -m "Phase 1: Thread-per-request (before Phase 2)"
git push origin learning/phase-1-threading
```

**Step 3-7: Repeat for each phase**

#### Learning Path

Students progress through branches:

1. **Start**: `learning/phase-0-baseline`
   - Implement threading (TODO #2)
   - Measure performance
   - Compare with `learning/phase-1-threading`

2. **Continue**: `learning/phase-1-threading`
   - Implement async I/O (TODOs #12, #13)
   - Measure performance
   - Compare with `learning/phase-2-async-io`

3. **Repeat** through all phases

4. **Final**: Compare with `main` branch

---

### Option B: Single Learning Branch

One branch with all TODOs, no implementations.

#### Branch Structure

```
main                    # Full implementation
learning/all-todos      # All TODOs, minimal implementations
```

#### Implementation Plan

```bash
git checkout -b learning/all-todos main

# Remove implementations for:
# - Rate limiting (leave TODO #8)
# - Async I/O (leave TODOs #12, #13)
# - Optimized JSON parser (leave TODO #10)
# - Hash-based mutex (leave TODO #15)
# - Lock-free ring buffer (leave TODOs #16-18)
# - Thread pool (leave TODOs #19-22)

# Keep working code for:
# - Basic HTTP server
# - Request routing
# - Validation

git commit -m "Learning branch: All TODOs for implementation"
git push origin learning/all-todos
```

#### Learning Path

Students:
1. Fork `learning/all-todos`
2. Implement TODOs in any order
3. Compare with `main` for reference

**Pros**:
- Simple single branch
- Choose your own adventure

**Cons**:
- No progressive difficulty
- Hard to measure incremental progress

---

### Option C: Tag-Based Milestones

Use git tags on `main` to mark phase boundaries.

#### Tag Structure

```
git tag learning/phase-0-baseline <commit>
git tag learning/phase-1-complete <commit>
git tag learning/phase-2-complete <commit>
# etc.
```

#### Learning Path

```bash
# Checkout specific phase
git checkout learning/phase-1-complete

# See what's implemented up to Phase 1
# Implement next phase
# Compare with next tag
```

**Pros**:
- No branch maintenance
- Clear milestones

**Cons**:
- Requires commits for each phase (not available in current repo)

---

## Comparison Matrix

| Strategy | Pros | Cons | Best For |
|----------|------|------|----------|
| **Option A: Progressive Branches** | Clear progression, measurable milestones, isolated phases | Multiple branches to maintain | Structured learning, bootcamps |
| **Option B: Single Learning Branch** | Simple, flexible order | No guidance, hard to measure progress | Self-directed learners |
| **Option C: Tag-Based** | Minimal maintenance | Requires phase commits | Reference only |

---

## Recommended Implementation: Option A

### Branch Creation Script

```bash
#!/bin/bash
# create_learning_branches.sh

# Phase 0: Baseline (minimal HTTP server)
git checkout -b learning/phase-0-baseline main
# [Manual: Remove all optimizations]
git commit -m "Learning Phase 0: Baseline HTTP server

Includes:
- Basic socket server
- HTTP request/response parsing
- Simple routing
- Synchronous single-threaded handling

TODOs to implement:
- Rate limiting
- JSON parsing
- File I/O
- Concurrency optimizations"
git push origin learning/phase-0-baseline

# Phase 1: Threading
git checkout -b learning/phase-1-threading learning/phase-0-baseline
# [Manual: Add thread-per-request, keep rest as TODOs]
git commit -m "Learning Phase 1: Thread-per-request

Implemented:
- Thread-per-request model
- Concurrent request handling

Next TODOs:
- Async I/O (Phase 2)
- JSON optimization (Phase 3)
- etc."
git push origin learning/phase-1-threading

# Repeat for phases 2-6...
```

---

## Branch Content Guide

### Phase 0: Baseline
**Implemented**:
- ✅ HTTP server (blocking)
- ✅ Request routing
- ✅ Metric validation

**TODOs**:
- ❌ Rate limiting (#8)
- ❌ JSON parsing (#9, #10)
- ❌ Async I/O (#12, #13)
- ❌ Thread pool (#19-22)
- ❌ Lock-free (#16-18)

### Phase 1: Threading
**Implemented**:
- ✅ Phase 0 +
- ✅ Thread-per-request

**TODOs**:
- ❌ Async I/O (#12, #13)
- ❌ JSON optimization (#10)
- ❌ Rate limiting (#8)
- ❌ Lock-free (#16-18)
- ❌ Thread pool (#19-22)

### Phase 2: Async I/O
**Implemented**:
- ✅ Phases 0-1 +
- ✅ Async write queue (#12)
- ✅ Background writer (#13)

**TODOs**:
- ❌ JSON optimization (#10)
- ❌ Rate limiting (#8)
- ❌ Lock-free (#16-18)
- ❌ Thread pool (#19-22)

### Phase 3: JSON Parsing
**Implemented**:
- ✅ Phases 0-2 +
- ✅ Optimized JSON parser (#10)

**TODOs**:
- ❌ Rate limiting (#8)
- ❌ Hash-based mutex (#15)
- ❌ Lock-free (#16-18)
- ❌ Thread pool (#19-22)

### Phase 4: Mutex Pool
**Implemented**:
- ✅ Phases 0-3 +
- ✅ Rate limiting (#8)
- ✅ Hash-based mutex pool (#15)

**TODOs**:
- ❌ Lock-free metrics (#16-18)
- ❌ Thread pool (#19-22)

### Phase 5: Lock-Free
**Implemented**:
- ✅ Phases 0-4 +
- ✅ Lock-free ring buffer (#16)
- ✅ Lock-free write (#17)
- ✅ Lock-free read (#18)

**TODOs**:
- ❌ Thread pool (#19-22)

### Phase 6: Thread Pool
**Implemented**:
- ✅ Phases 0-5 +
- ✅ Thread pool (#19-21)
- ✅ HTTP integration (#22)

**This is current `main` branch**

---

## Learning Hub Integration

### Lesson Mapping

Each lesson in the Learning Hub links to appropriate branch:

**Lesson 1-3: Foundations**
→ Branch: `learning/phase-0-baseline`

**Lesson 4-6: Concurrency Basics**
→ Branch: `learning/phase-1-threading`

**Lesson 7-8: Async I/O**
→ Branch: `learning/phase-2-async-io`

**Lesson 9-10: Performance**
→ Branch: `learning/phase-3-json-parsing`

**Lesson 11-12: Advanced Concurrency**
→ Branch: `learning/phase-4-mutex-pool`

**Lesson 13-14: Lock-Free**
→ Branch: `learning/phase-5-lock-free`

**Lesson 15: Thread Pool**
→ Branch: `learning/phase-6-thread-pool` (or `main`)

### Exercise Instructions

Each exercise provides:

```markdown
## Setup

1. Clone the repository:
   git clone https://github.com/user/MetricsStream.git

2. Checkout the learning branch:
   git checkout learning/phase-2-async-io

3. Create your work branch:
   git checkout -b my-phase-3-solution

4. Implement TODOs in TODOS.md

5. Test your implementation:
   ./build/load_test 8080 100 10

6. Compare with next phase:
   git diff learning/phase-3-json-parsing

7. Measure performance gain:
   ./performance_test.sh
```

---

## Maintenance Strategy

### Updating Learning Branches

When `main` changes (bug fixes, new features):

```bash
# Option 1: Cherry-pick fixes
git checkout learning/phase-0-baseline
git cherry-pick <bugfix-commit>
git push origin learning/phase-0-baseline

# Option 2: Rebase (careful!)
git checkout learning/phase-0-baseline
git rebase main
# Resolve conflicts
git push --force-with-lease origin learning/phase-0-baseline

# Option 3: Recreate (if major changes)
# Delete old branch, create new from main
```

### Automation

```bash
# Script to sync all learning branches
for phase in 0 1 2 3 4 5 6; do
    git checkout learning/phase-${phase}-*
    git cherry-pick <fix-commit>
    git push origin learning/phase-${phase}-*
done
```

---

## Branch Naming Convention

Format: `learning/<phase>-<name>`

Examples:
- `learning/phase-0-baseline`
- `learning/phase-1-threading`
- `learning/phase-2-async-io`
- `learning/phase-3-json-parsing`
- `learning/phase-4-mutex-pool`
- `learning/phase-5-lock-free`
- `learning/phase-6-thread-pool`

**Alternative naming** (for multiple difficulty paths):
- `learning/beginner/phase-1`
- `learning/intermediate/phase-3`
- `learning/advanced/phase-5`

---

## README for Learning Branches

Each branch should have updated README:

```markdown
# MetricsStream - Learning Branch: Phase 2 (Async I/O)

## This Branch

You are on the Phase 2 learning branch. This code includes:
- ✅ Phase 0: HTTP server basics
- ✅ Phase 1: Thread-per-request
- 🚧 Phase 2: Async I/O (YOUR TASK)

## Your Mission

Implement async I/O with producer-consumer pattern:
1. TODO #12: Async write queue
2. TODO #13: Background writer thread

## Performance Baseline

Before your changes:
- 50 clients: ~60% success rate
- File I/O blocks HTTP threads

## Performance Target

After your changes:
- 50 clients: 66%+ success rate
- HTTP threads non-blocking

## Testing

```bash
# Build
mkdir build && cd build
cmake .. && make

# Test
./load_test 8080 50 10

# Measure
./performance_test.sh
```

## Hints

See `TODOS.md` for detailed requirements.
See `LEARNING_PATH.md` for implementation guide.

## Stuck?

Compare with next phase:
```bash
git diff learning/phase-3-json-parsing
```

Or check reference implementation in `main`.

## Next Phase

When ready, move to:
```bash
git checkout learning/phase-3-json-parsing
```
```

---

## Conclusion

**Recommended approach**: **Option A - Progressive Learning Branches**

**Why**:
- Clear learning progression
- Measurable milestones
- Isolated complexity per phase
- Supports structured curriculum

**Next steps**:
1. Create Phase 0 baseline branch
2. Incrementally add phases 1-6
3. Update Learning Hub to reference branches
4. Document branch strategy in main README

This approach transforms the monolithic main branch into a structured learning journey!
