#include "SiteProfiles.hpp"

const char *siteProfilesEditorScript() {
  static const char script[] PROGMEM = R"HTML(<script>
(()=>{
const hidden=document.getElementById('siteProfilesData');if(!hidden)return;
const form=hidden.form,choose=document.getElementById('siteProfileChoice'),hint=document.getElementById('siteProfileHint'),group=document.getElementById('siteProfileFields');
const english=document.documentElement.lang==='en',text=(de,en)=>english?en:de;
let model=JSON.parse(hidden.value),airports=JSON.parse(document.getElementById('siteAirportsData').value),request=0;
const fields={name:form.elements.namedItem('siteName'),latitude:form.elements.namedItem('siteLat'),longitude:form.elements.namedItem('siteLon'),elevation_m:form.elements.namedItem('observerElevation'),view_heading:form.elements.namedItem('viewHeading')};
// Keep the existing elevation input, but show all profile fields together.
if(group&&fields.elevation_m)group.append(fields.elevation_m.closest('label'));
for(const input of Object.values(fields)){
 if(input.type==='number')input.required=true;
 input.addEventListener('input',()=>{input.setCustomValidity('');input.removeAttribute('aria-invalid')});
 input.addEventListener('invalid',()=>{input.setAttribute('aria-invalid','true');hint.textContent=text('Bitte die markierten Standortangaben ausfüllen oder korrigieren.','Please complete or correct the highlighted site fields.')});
}
function sync(){hidden.value=JSON.stringify(model)}
function options(){
 choose.replaceChildren();const own=document.createElement('optgroup');own.label=text('Eigene Standorte','Saved sites');
 model.profiles.forEach((p,i)=>own.append(new Option(p.name||text('Standort ','Site ')+(i+1),'p:'+i)));
 choose.append(own);const near=document.createElement('optgroup');near.label=text('Flughafen-Vorlagen · 150 km','Airport templates · 150 km');
 [...airports].sort((a,b)=>a.distance_km-b.distance_km).forEach(a=>near.append(new Option(a.name+' ('+a.code+') · '+a.distance_km.toFixed(0)+' km','a:'+a.code)));
 choose.append(near);choose.value='p:'+model.active;sync();
}
function fill(){const p=model.profiles[model.active];for(const [k,input] of Object.entries(fields))input.value=p[k];options()}
function remember(){
 for(const input of Object.values(fields)){
  const missing=input.type==='number'&&(input.value.trim()===''||!Number.isFinite(Number(input.value)));
  input.setCustomValidity(missing?text('Bitte einen gültigen Zahlenwert eingeben.','Please enter a valid number.'):'');
  if(!input.checkValidity()){input.reportValidity();return false}
 }
 model.profiles[model.active]=Object.fromEntries(Object.entries(fields).map(([k,input])=>[k,k==='name'?input.value:Number(input.value)]));sync();return true;
}
function pending(){hint.textContent=text('Änderungen werden erst mit „Einstellungen speichern“ aktiviert.','Changes take effect only after Save settings.')}
async function nearby(){
 const serial=++request,lat=Number(fields.latitude.value),lon=Number(fields.longitude.value);
 airports=[];options();
 if(!Number.isFinite(lat)||!Number.isFinite(lon)||Math.abs(lat)>90||Math.abs(lon)>180)return;
 try{const r=await fetch('/api/site-airports?lat='+encodeURIComponent(lat)+'&lon='+encodeURIComponent(lon),{cache:'no-store'});if(!r.ok)throw Error();const rows=await r.json();if(serial!==request)return;airports=rows;options()}
 catch(e){if(serial===request){airports=[];options();hint.textContent=text('Flughafen-Vorlagen konnten nicht geladen werden.','Airport templates could not be loaded.')}}
}
choose.addEventListener('change',()=>{
 const selected=choose.value;if(!remember()){choose.value='p:'+model.active;return}
 if(selected.startsWith('p:'))model.active=Number(selected.slice(2));
 else {
  if(model.profiles.length>=16){hint.textContent=text('Maximal 16 Standorte.','Maximum 16 sites.');choose.value='p:'+model.active;return}
  const a=airports.find(a=>'a:'+a.code===selected);if(!a)return;
  const previous=model.profiles[model.active];model.profiles.push({name:a.name+' ('+a.code+')',latitude:a.latitude,longitude:a.longitude,elevation_m:a.elevation_m??previous.elevation_m,view_heading:previous.view_heading});model.active=model.profiles.length-1;
 }
 fill();pending();nearby();
});
document.getElementById('siteProfileNew').addEventListener('click',()=>{
 if(!remember())return;if(model.profiles.length>=16){hint.textContent=text('Maximal 16 Standorte.','Maximum 16 sites.');return}
 model.profiles.push({...model.profiles[model.active],name:''});model.active=model.profiles.length-1;fill();fields.name.focus();pending();
});
document.getElementById('siteProfileApply').addEventListener('click',()=>{if(remember()){options();pending();nearby()}});
document.getElementById('siteProfileDelete').addEventListener('click',()=>{
 if(model.profiles.length===1){hint.textContent=text('Mindestens ein Standort muss erhalten bleiben.','At least one site must remain.');return}
 if(!confirm(text('Diesen Standort aus der Liste entfernen?','Remove this site from the list?')))return;
 model.profiles.splice(model.active,1);model.active=Math.min(model.active,model.profiles.length-1);fill();pending();nearby();
});
form.addEventListener('submit',e=>{if(!remember())e.preventDefault()});
for(const input of [fields.latitude,fields.longitude])input.addEventListener('change',nearby);
options();
})();
</script>)HTML";
  return script;
}
