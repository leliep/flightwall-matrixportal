#!/usr/bin/env python3
"""Run the actual site editor in headless Chrome with local, isolated fixtures."""
from pathlib import Path
import argparse,re,json,html,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--chrome',default='/Applications/Google Chrome.app/Contents/MacOS/Google Chrome');args=parser.parse_args()
s=(root/'src/main.cpp').read_text();script=re.search(r'R"HTML\((.*?)\)HTML"',(root/'src/SiteProfilesUi.cpp').read_text(),re.S)[1]
start=s.index("<div class='fieldGrid'><label class='fieldWide'>Standortprofil");end=s.index("<div class='fieldGrid' id='siteProfileFields'>",start)
controls=s[start:end].replace('\\"','"')
model={'profiles':[{'name':'Original','latitude':51.8,'longitude':6.1,'elevation_m':42,'view_heading':290}],'active':0}
airports=[{'code':'NRN','name':'Weeze','latitude':51.6,'longitude':6.14,'elevation_m':32,'distance_km':22}]
controls=controls.replace('%FW_SITE_PROFILES%',html.escape(json.dumps(model),quote=True)).replace('%FW_SITE_AIRPORTS%',html.escape(json.dumps(airports),quote=True))
fields="<div id='siteProfileFields'>"
for key,value in [('siteName','Original'),('siteLat','51.8'),('siteLon','6.1'),('viewHeading','290')]:
 fields+=f"<label>{key}<input name='{key}' value='{value}' type='{'text' if key=='siteName' else 'number'}' step='any'></label>"
fields+="</div>"
fixture="<!doctype html><html lang='de'><meta charset='utf-8'><style>*{box-sizing:border-box}body{margin:16px}input,select{max-width:100%}.buttonRow{display:flex;flex-wrap:wrap;gap:8px}label{display:block;margin-bottom:8px}</style><form id='config'><label>Height<input type='number' name='observerElevation' step='any' value='42'></label><input name='radius' value='30'>"+controls+fields+"<button id='submit'>Save</button></form>"
stub='<script>window.fetch=async()=>({ok:true,json:async()=>'+json.dumps(airports)+'''});window.confirm=()=>true;</script>'''
check=r'''<script>
setTimeout(async()=>{try{
const by=id=>document.getElementById(id),form=by('config'),field=n=>form.elements.namedItem(n),model=()=>JSON.parse(by('siteProfilesData').value),choose=by('siteProfileChoice');
const change=value=>{choose.value=value;choose.dispatchEvent(new Event('change'))};
const wait=()=>new Promise(r=>setTimeout(r,20));
const assert=(ok,msg)=>{if(!ok)throw Error(msg)};
assert(model().profiles.length===1&&field('siteName').value==='Original','initial migration');
assert(by('siteProfileFields').contains(field('observerElevation')),'height grouped with profile');
change('a:NRN');await wait();assert(field('siteName').value==='Weeze (NRN)'&&Number(field('siteLat').value)===51.6&&Number(field('observerElevation').value)===32&&Number(field('viewHeading').value)===290,'airport values and retained heading');
by('siteProfileNew').click();field('siteName').value='<script> & Büro';field('siteLat').value='52.1';field('observerElevation').value='105';field('viewHeading').value='45';by('siteProfileApply').click();await wait();
assert(model().profiles.length===3&&model().profiles[2].name==='<script> & Büro','custom site');
change('p:0');await wait();assert(field('siteName').value==='Original'&&Number(field('observerElevation').value)===42,'switch back');
change('p:2');await wait();assert(Number(field('observerElevation').value)===105&&Number(field('viewHeading').value)===45,'restore custom profile fields');
by('siteProfileDelete').click();await wait();assert(model().profiles.length===2,'delete');
change('p:0');await wait();
while(model().profiles.length<16){by('siteProfileNew').click();field('siteName').value='Site '+model().profiles.length;by('siteProfileApply').click()}
by('siteProfileNew').click();assert(model().profiles.length===16,'limit');
form.addEventListener('submit',e=>e.preventDefault());field('siteName').value='Final';form.dispatchEvent(new Event('submit',{cancelable:true}));
assert(model().profiles[model().active].name==='Final','submit sync');assert(field('radius').value==='30','unrelated settings preserved');
const result=document.createElement('pre');result.id='profile-test-result';result.textContent=JSON.stringify({passed:true,profiles:model().profiles.length,overflow:document.documentElement.scrollWidth>innerWidth});document.body.append(result);
}catch(e){const result=document.createElement('pre');result.id='profile-test-result';result.textContent=JSON.stringify({passed:false,error:e.message});document.body.append(result)}},100);
</script>'''
# Avoid a literal closing script tag within the test's string data.
check=check.replace("'<script> & Büro'","'<scr'+'ipt> & Büro'")
with tempfile.TemporaryDirectory(prefix='flightwall-profile-browser-') as temp:
 p=Path(temp);(p/'test.html').write_text(fixture+stub+script+check)
 for width in [1280,390]:
  r=subprocess.run([args.chrome,'--headless','--no-first-run','--no-default-browser-check','--disable-gpu','--user-data-dir='+str(p/('profile-'+str(width))),'--window-size='+str(width)+',1000','--virtual-time-budget=1000','--dump-dom',(p/'test.html').as_uri()],capture_output=True,text=True,timeout=40,check=True)
  match=re.search(r'<pre id="profile-test-result">(.*?)</pre>',r.stdout);assert match,r.stdout[-1000:]
  result=json.loads(match[1]);assert result['passed'] and not result['overflow'],result
  print(width,result,flush=True)
