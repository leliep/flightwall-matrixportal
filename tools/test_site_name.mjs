// Run with Node.js: node tools/test_site_name.mjs
import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const source=fs.readFileSync(new URL('../src/WebDisplay.cpp',import.meta.url),'utf8');
const script=source.match(/<script>([\s\S]*?)<\/script>/)[1];
const label={hidden:true,textContent:''},content={innerHTML:''};
Object.defineProperty(label,'innerHTML',{set(){throw new Error('Site name must remain plain text')}});
const sandbox=vm.createContext({document:{getElementById:id=>({siteName:label,content}[id])}});
new vm.Script(script); // Complete browser code remains syntactically valid.
vm.runInContext(script.slice(0,script.indexOf('setInterval(update,2000)')),sandbox);
const render=name=>{sandbox.state={site_name:name,flight:{available:false}};vm.runInContext('render(state)',sandbox)};
render('Büro 2 – München');assert.equal(label.textContent,'Büro 2 – München');assert.equal(label.hidden,false);
render('<img src=x onerror=alert(1)>');assert.equal(label.textContent,'<img src=x onerror=alert(1)>');
assert.equal(label.hidden,false); // Works without aircraft data, no HTML injection.
render('');assert.equal(label.hidden,true);assert.equal(label.textContent,'');
render(undefined);assert.equal(label.hidden,true); // Compatible with older API payloads.
console.log('Site name web display: plain text, empty/missing value and no-aircraft state passed');
