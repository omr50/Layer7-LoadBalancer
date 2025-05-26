const http = require('http');
const fs = require('fs');
const { performance } = require('perf_hooks');

const TARGET = { host: 'localhost', port: 8000, path: '/', method: 'GET' };
const TOTAL_REQUESTS = 100000;
const CONCURRENCY = 600;
const OUTPUT_FILE = 'responses.log';

let completed = 0;
let started = 0;
let failed = 0;

const responses = new Array(TOTAL_REQUESTS);
const agent = new http.Agent({
    keepAlive: true,
    maxSockets: CONCURRENCY,
    keepAliveMsecs: 1000
});

const start = performance.now();

function makeRequest(i) {
    return new Promise((resolve) => {
        const options = {
            ...TARGET,
            agent,
            headers: {
                // Request keep-alive; let server decide when to close
                'Connection': 'keep-alive'
            }
        };
        const req = http.request(options, (res) => {
            let data = '';
            // Gather headers and status
            let fullResponse = `HTTP/${res.httpVersion} ${res.statusCode} ${res.statusMessage}\r\n`;
            for (const [k, v] of Object.entries(res.headers)) {
                fullResponse += `${k}: ${v}\r\n`;
            }
            fullResponse += '\r\n';

            res.setEncoding('utf8');
            res.on('data', chunk => { data += chunk; });
            res.on('end', () => {
                fullResponse += data;
                responses[i] = fullResponse;
                completed++;
                resolve();
            });
        });
        req.on('error', (e) => {
            responses[i] = `ERROR: ${e.message}`;
            failed++;
            completed++;
            resolve();
        });
        req.end();
    });
}

// Request scheduler
async function main() {
    // Open write stream for concurrent logging
    const writeStream = fs.createWriteStream(OUTPUT_FILE, { flags: 'a' });

    let inflight = 0;
    let next = 0;
    let promises = [];

    // Kick off up to CONCURRENCY requests
    function kick() {
        while (inflight < CONCURRENCY && next < TOTAL_REQUESTS) {
            inflight++;
            const myIndex = next++;
            makeRequest(myIndex).then(() => {
                inflight--;
                kick();
            });
        }
    }

    kick();

    // Wait until all are done
    while (completed < TOTAL_REQUESTS) {
        await new Promise(r => setTimeout(r, 100));
    }

    // Write all responses to file, as fast as possible
    for (const resp of responses) {
        if (resp) writeStream.write(resp + "\n===\n");
    }
    writeStream.end();

    const end = performance.now();
    console.log(`Completed ${TOTAL_REQUESTS} requests in ${((end-start)/1000).toFixed(2)} seconds`);
    console.log(`Failed requests: ${failed}`);
}

main();
