// Run with Node.js: node tools/test_web_radar.mjs
import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const source = fs.readFileSync(new URL('../src/WebDisplay.cpp', import.meta.url), 'utf8');
const script = source.match(/<script>([\s\S]*?)<\/script>/)[1];
let now = 0;
const draws = [];
const context = {globalAlpha:1, fillStyle:'', fillRect(x,y,w,h){draws.push({x,y,w,h,alpha:this.globalAlpha,color:this.fillStyle})}};
const elements = Object.fromEntries(['radarPanel','radarCanvas','radarMeta'].map(id => [id, {hidden:false,getContext:()=>context}]));
const sandbox = vm.createContext({document:{getElementById:id=>elements[id]},performance:{now:()=>now}, Date, console});
// Compile the complete browser script, but omit its automatic polling startup.
new vm.Script(script);
vm.runInContext(script.slice(0,script.indexOf('setInterval(update,2000)')),sandbox);
const state = {web_radar:true,radar_trails_enabled:true,radarReceivedAt:0,radar:{radius_km:10,view_heading:0,points:[[5,90,1,[[60,10,60000],[59,10,10000]]]]}};
sandbox.state=state;
const draw=()=>{draws.length=0;vm.runInContext('drawRadar(state)',sandbox)};
draw();
assert(draws.some(p=>p.x===60 && p.y===10 && p.alpha===0.43));
assert(draws.some(p=>p.x===59 && p.y===10 && p.alpha===0.15));
assert(draws.findIndex(p=>p.x===60&&p.y===10) < draws.findIndex(p=>p.x===72&&p.y===15));
assert.equal(context.globalAlpha,1);
now=10001;draw();
assert(!draws.some(p=>p.x===59 && p.y===10));
now=60000;draw();
assert(!draws.some(p=>p.x===60 && p.y===10));
now=0;state.radar_trails_enabled=false;draw();
assert(!draws.some(p=>p.x===60 && p.y===10));
state.radar_trails_enabled=true;state.radar_trails_available=false;draw();
assert(!draws.some(p=>p.x===60 && p.y===10));
assert(elements.radarMeta.textContent.includes('Spuren nicht verfügbar'));
state.radar_trails_available=true;draw();
assert(draws.some(p=>p.x===60 && p.y===10));
state.web_radar=false;draw();assert(elements.radarPanel.hidden);
state.web_radar=true;state.radar_trails_enabled=true;state.radar.points=[[5,90,1]];
draw();assert.equal(elements.radarPanel.hidden,false); // legacy points without trails
console.log('Web radar tests passed: fading, expiry without new data, disabled state, layer order, legacy payload');

// Sweep timing and layering use the actual browser implementation.
const lastPixel=(x,y)=>draws.filter(p=>p.x===x&&p.y===y&&p.w===1&&p.h===1).at(-1);
state.radar.radius_km=15;state.radar.points=[];state.radar_sweep_style='line';state.radar_sweep_interval_seconds=5;now=0;draw();
assert.equal(lastPixel(64,0).color,'#0b552b');
now=194;draw();assert.equal(lastPixel(79,15).color,'#0b552b');
now=388;draw();assert.equal(lastPixel(64,30).color,'#0b552b');
now=581;draw();assert.equal(lastPixel(49,15).color,'#0b552b');
now=775;draw();assert.equal(lastPixel(64,0).color,'#0b552b');
now=800;draw();assert(!draws.some(p=>p.color==='#0b552b'));
now=5000;draw();assert.equal(lastPixel(64,0).color,'#0b552b');
// Trails and live targets cannot be covered by the beam.
state.radar.points=[[10,90,1,[[72,15,60000]]]];state.radarReceivedAt=5194;now=5194;draw();
assert.equal(lastPixel(74,15).color,'#ffd166');assert.equal(lastPixel(72,15).color,'#ffd166');
assert(draws.findIndex(p=>p.x===72&&p.y===15&&p.color==='#0b552b')<draws.findIndex(p=>p.x===72&&p.y===15&&p.color==='#ffd166'));
state.radar_sweep_interval_seconds=0;draw();assert(!draws.some(p=>p.color==='#0b552b'||p.color==='#f3f8fc'));
state.radar_sweep_style='dot';state.radar_sweep_interval_seconds=1;state.radar.points=[];now=6000;draw();
assert.equal(lastPixel(64,0).color,'#f3f8fc');assert(!draws.some(p=>p.color==='#0b552b'));
now=6775;draw();assert.equal(lastPixel(64,0).color,'#f3f8fc');
now=6800;draw();assert(!draws.some(p=>p.color==='#f3f8fc'));
now=7000;draw();assert.equal(lastPixel(64,0).color,'#f3f8fc');
state.web_radar=false;draw();state.web_radar=true;now=7100;draw();assert.equal(lastPixel(64,0).color,'#f3f8fc');
assert.equal(vm.runInContext('radarSweepNextDelay',sandbox),25);
console.log('Web sweep tests passed: dot/beam, clockwise endpoints, period/off, restart, trail/target priority');
