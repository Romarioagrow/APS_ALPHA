const fs = require('fs');
const path = require('path');

const fragmentPath = path.join(__dirname, 'aposfera-ui-evolution.html');
const fragment = fs.readFileSync(fragmentPath, 'utf8');
const scriptMatch = fragment.match(/<script>([\s\S]*?)<\/script>/);

if (!scriptMatch) throw new Error('Embedded interaction script is missing.');
new Function(scriptMatch[1]);

const uniqueMatches = (pattern) => [...new Set([...fragment.matchAll(pattern)].map((match) => match[1]))];
const callouts = uniqueMatches(/data-callout="([^"]+)"/g);
const annotated = uniqueMatches(/data-annotated="([^"]+)"/g);
const screens = uniqueMatches(/data-screen="([^"]+)"/g);
const viewButtons = uniqueMatches(/data-view="([^"]+)"/g);
const missingTargets = callouts.filter((name) => !annotated.includes(name));
const missingViews = viewButtons.filter((name) => !screens.includes(name));

if (!fragment.includes('id="aposfera-ui-evolution-mockup"')) throw new Error('Unique fragment root is missing.');
if (fragment.length >= 1024 * 1024) throw new Error('Fragment exceeds the 1 MB limit.');
if (missingTargets.length) throw new Error(`Missing callout targets: ${missingTargets.join(', ')}`);
if (missingViews.length) throw new Error(`Missing screen targets: ${missingViews.join(', ')}`);

console.log('MOCKUP_STATIC=PASS');
console.log(`BYTES=${Buffer.byteLength(fragment, 'utf8')}`);
console.log(`SCREENS=${screens.join(',')}`);
console.log(`CALLOUTS=${callouts.join(',')}`);

