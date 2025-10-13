# Phase 0 Quick Start (5 minutes)

Get a complete monitoring platform running in 5 minutes.

## 1. Build (1 minute)

```bash
cd phase0
./build.sh
```

## 2. Run Server (30 seconds)

Terminal 1:
```bash
cd build
./phase0_poc
```

## 3. Test It (3 minutes)

Terminal 2:
```bash
# Send metrics
curl -X POST http://localhost:8080/metrics \
     -d '{"name":"cpu_usage","value":85}'

# Query metrics
curl "http://localhost:8080/query?name=cpu_usage&start=0&end=9999999999999"

# Check health
curl http://localhost:8080/health

# View stored data
cat build/phase0_metrics.jsonl
```

## 4. See Alerts (30 seconds)

Send high CPU values to trigger alert:

```bash
for i in {1..5}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d '{"name":"cpu_usage","value":95}' > /dev/null
  sleep 1
done
```

Wait 10 seconds, then check server logs for:
```
🚨 [ALERT] cpu_usage avg(60s) > 80.0
```

## What's Running?

✅ **Ingestion API** - Accepting metrics on port 8080
✅ **In-Memory Queue** - Buffering metrics thread-safely
✅ **Storage Consumer** - Writing to phase0_metrics.jsonl
✅ **Query API** - Serving GET /query requests
✅ **Alerting Engine** - Checking rules every 10s

## Next Steps

- **Full Demo:** `./demo.sh` (automated test)
- **Tutorial:** Read TUTORIAL.md for deep dive
- **Optimize:** Move to Craft #1 for 10x performance

## Endpoints

```
POST /metrics           - Ingest metric
GET  /query?name=...    - Query metrics
GET  /health            - Health check
```

---

**Time to working system:** 5 minutes
**Time to understand it:** 2-3 hours (TUTORIAL.md)
