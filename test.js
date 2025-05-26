// Save as stress_test.js
const http = require('http');
const fs = require('fs');
const { performance } = require('perf_hooks');

// ============= CONFIG =============
const TARGET = 'http://localhost:8000'; // Change as needed
const CONCURRENCY = 20;                // Number of in-flight requests
const DURATION_SEC = 10;                // Total run time
const OUTPUT_FILE = 'results.log';
// ===================================

let running = true;
let completed = 0;
let started = 0;
let errors = 0;
let latencies = [];
let reqsInLastSecond = 0;
let lastSecond = Math.floor(performance.now() / 1000);

const writeStream = fs.createWriteStream(OUTPUT_FILE, { flags: 'a' });

// Print RPS every second
setInterval(() => {
    const now = Math.floor(performance.now() / 1000);
    if (now !== lastSecond) {
        console.log(`RPS: ${reqsInLastSecond}`);
        reqsInLastSecond = 0;
        lastSecond = now;
    }
}, 1000);

// Make one request, return a Promise
function makeRequest(id) {
    return new Promise((resolve) => {
        const start = performance.now();
        const req = http.get(TARGET, (res) => {
            let data = '';
            res.on('data', chunk => { data += chunk; });
            res.on('end', () => {
                const latency = performance.now() - start;
                latencies.push(latency);
                reqsInLastSecond++;
                completed++;
                const result = `Request#${id} status=${res.statusCode} latency=${latency.toFixed(2)}ms\n`;
                writeStream.write(result);
                process.stdout.write(result); // print to console
                resolve();
            });
        });
        req.on('error', (e) => {
            errors++;
            reqsInLastSecond++;
            const latency = performance.now() - start;
            const result = `Request#${id} error latency=${latency.toFixed(2)}ms: ${e.message}\n`;
            writeStream.write(result);
            process.stdout.write(result);
            resolve();
        });
    });
}

// Keep launching requests up to concurrency limit
async function startFlood() {
    let inflight = 0;
    let nextId = 1;
    const promises = [];

    const endTime = Date.now() + DURATION_SEC * 1000;

    async function launch() {
        while (running && Date.now() < endTime) {
            if (inflight < CONCURRENCY) {
                inflight++;
                started++;
                makeRequest(nextId++).then(() => {
                    inflight--;
                });
            } else {
                // Throttle just enough to avoid starving the event loop
                await new Promise(r => setTimeout(r, 1));
            }
        }
        running = false;
    }

    await launch();

    // Wait for all in-flight requests to finish
    while (inflight > 0) {
        await new Promise(r => setTimeout(r, 10));
    }
    writeStream.end();
}

// Run the test
(async () => {
    console.log(`Starting stress test: ${CONCURRENCY} concurrency for ${DURATION_SEC}s to ${TARGET}`);
    await startFlood();

    // Calculate statistics
    const avg = latencies.length ? (latencies.reduce((a, b) => a + b) / latencies.length) : 0;
    const max = latencies.length ? Math.max(...latencies) : 0;
    const min = latencies.length ? Math.min(...latencies) : 0;

    console.log('\n=== SUMMARY ===');
    console.log(`Total Requests Started: ${started}`);
    console.log(`Total Requests Completed: ${completed}`);
    console.log(`Total Errors: ${errors}`);
    console.log(`Avg Latency: ${avg.toFixed(2)}ms, Min: ${min.toFixed(2)}ms, Max: ${max.toFixed(2)}ms`);
    console.log(`Requests/sec: ${(completed / DURATION_SEC).toFixed(2)}`);
})();
