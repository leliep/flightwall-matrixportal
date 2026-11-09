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
state.web_radar=false;draw();assert(elements.radarPanel.hidden);
state.web_radar=true;state.radar_trails_enabled=true;state.radar.points=[[5,90,1]];
draw();assert.equal(elements.radarPanel.hidden,false); // legacy points without trails
console.log('Web radar tests passed: fading, expiry without new data, disabled state, layer order, legacy payload');
