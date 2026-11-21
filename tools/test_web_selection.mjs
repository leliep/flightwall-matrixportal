// Run: node tools/test_web_selection.mjs
import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const source=fs.readFileSync(new URL('../src/WebDisplay.cpp',import.meta.url),'utf8');
const script=source.match(/<script>([\s\S]*?)<\/script>/)[1];
let now=0,requests=[];
const elements=Object.fromEntries(['radarSelectionHint','radarAutomatic'].map(id=>[id,{hidden:true,textContent:''}]));
const context=vm.createContext({document:{getElementById:id=>elements[id]},performance:{now:()=>now},Date:{now:()=>0},console,
 fetch:async(url,options)=>{requests.push({url,options});return {ok:true}}});
vm.runInContext(script.slice(0,script.indexOf('setInterval(update,2000)')),context);
context.s={web_radar:true,radarReceivedAt:0,radar:{radius_km:15,view_heading:0,points:[[10,90,0,[],'123456789abcdef1'],[10,270,1,[],'123456789abcdef2']]},manual_selection:{active:true,id:'123456789abcdef1',remaining_seconds:60},flight:{emergency:false}};
context.rect={left:50,top:20,width:512,height:128};
assert.equal(vm.runInContext('radarHit(s,348,82,rect)',context),'123456789abcdef1');
assert.equal(vm.runInContext('radarHit(s,268,82,rect)',context),'123456789abcdef2');
assert.equal(vm.runInContext('radarHit(s,400,120,rect)',context),null);
// Responsive hit testing uses screen pixels, not an enlarged logical hit radius.
context.rect={left:0,top:0,width:256,height:64};
assert.equal(vm.runInContext('radarHit(s,149,31,rect)',context),'123456789abcdef1');
assert.equal(vm.runInContext('radarHit(s,170,31,rect)',context),null);
now=15001;assert.equal(vm.runInContext('radarHit(s,149,31,rect)',context),null);
now=0;vm.runInContext('renderManualSelection(s)',context);assert.equal(elements.radarAutomatic.hidden,false);assert(elements.radarSelectionHint.textContent.includes('60 s'));
now=59001;vm.runInContext('renderManualSelection(s)',context);assert(elements.radarSelectionHint.textContent.includes('1 s'));
now=60000;vm.runInContext('renderManualSelection(s)',context);assert.equal(elements.radarAutomatic.hidden,true);
assert.equal(context.s.manual_selection.remaining_seconds,60); // polling/rendering never mutates or renews the server lease
vm.runInContext('update=async()=>{}',context);
await vm.runInContext('selectRadarAircraft("123456789abcdef1")',context);
assert.equal(requests[0].url,'/api/radar-selection');assert.equal(requests[0].options.method,'POST');assert.equal(requests[0].options.body,'id=123456789abcdef1');
await vm.runInContext('selectRadarAircraft("")',context);assert.equal(requests[1].options.body,'id=');
context.fetch=async()=>({ok:false});await vm.runInContext('selectRadarAircraft("123456789abcdef1")',context);assert(elements.radarSelectionHint.textContent.includes('nicht möglich'));
console.log('Web radar selection: scaled hit tests, missed/stale targets, countdown without lease refresh, POST/cancel and error handling passed');
