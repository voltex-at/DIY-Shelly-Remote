/* Frozen schema2 validator, including manual TRV target steps.
 * Do not extend this path when adding current-schema fields. */
static bool validate_v2(const cJSON *r,char error[96],bool stored)
{
 if(!cJSON_IsObject(r)||!unique_keys(r,0))return fail(error,"Invalid object or duplicate keys");
 if(!keys(r,"|schema||revision||language||theme||accent||active_dashboard||rooms||devices||scenes||dashboards||groups|"))return fail(error,"Unknown configuration field");
 if(!number(r,"schema",2,2)||!number(r,"revision",1,UINT32_MAX-1))return fail(error,"Unsupported schema/revision");
 if(!choice(r,"language","|pl||en||de|")||!theme(r,stored)||!number(r,"accent",0,0xffffff))return fail(error,"Invalid appearance");
 if(!collection(r,"rooms",8)||!collection(r,"devices",16)||!collection(r,"scenes",8)||!collection(r,"groups",8)||!collection(r,"dashboards",DSR_CONFIG_MAX_DASHBOARDS))return fail(error,"Invalid collection, duplicate ID or limit");
 if(!reference(r,"dashboards",get(r,"active_dashboard"),false))return fail(error,"Select an existing dashboard");
 const cJSON *v;
 cJSON_ArrayForEach(v,get(r,"rooms")){
  if(!keys(v,"|id||name||icon||photo|")||!number(v,"icon",0,3)||!text(v,"photo",32,true))return fail(error,"Invalid room");
  const char *photo=get(v,"photo")->valuestring;if(photo[0]){if(strlen(photo)!=32)return fail(error,"Invalid photo ID");for(unsigned i=0;i<32;i++)if(!((photo[i]>='a'&&photo[i]<='f')||(photo[i]>='0'&&photo[i]<='9')))return fail(error,"Invalid photo ID");}
 }
 cJSON_ArrayForEach(v,get(r,"devices")){
  const cJSON *host=get(v,"host");
  if(!keys(v,"|id||name||host||channel||kind||room||icon|")||(get(v,"icon")&&!number(v,"icon",0,DSR_CONFIG_DEVICE_ICONS))||!text(v,"host",15,true)||(host->valuestring[0]&&!dsr_ipv4_valid(host->valuestring))||!number(v,"channel",0,15)||
   !choice(v,"kind","|light||relay||plug||cover||contact||sensor|")||!reference(r,"rooms",get(v,"room"),true))return fail(error,"Invalid device address/type/room");
 }
 cJSON_ArrayForEach(v,get(r,"scenes")){
  const cJSON *actions=get(v,"actions"),*a;if(!keys(v,"|id||name||actions||room||photo|")||!action_appearance(r,v)||!cJSON_IsArray(actions)||cJSON_GetArraySize(actions)>8)return fail(error,"Scene limit: 8 explicit actions");
  /* Target values share the actor's half-degree representation (8..60).
   * Stored display kind cannot grant TRV capability; the executor checks the
   * actual private binding again before any write. Schema1 remains frozen. */
  cJSON_ArrayForEach(a,actions){
   if(!keys(a,"|device||command||value|")||!reference(r,"devices",get(a,"device"),false)||!choice(a,"command","|on||off||open||close||stop||target|"))return fail(error,"Invalid scene action");
   if(!strcmp(get(a,"command")->valuestring,"target")?!number(a,"value",DSR_TRV_MIN_HALF,DSR_TRV_MAX_HALF):get(a,"value")!=NULL)return fail(error,"Invalid scene value");
  }
 }
 /* Display kinds do not identify actuators (a BLU TRV is visually a sensor).
  * Member identity, homogeneous control and capability are checked against
  * private bindings by the executor before I/O, never granted by this schema. */
 cJSON_ArrayForEach(v,get(r,"groups")){
  const cJSON *members=get(v,"members"),*member;
  if(!keys(v,"|id||name||members||room||photo|")||!action_appearance(r,v)||!cJSON_IsArray(members)||cJSON_GetArraySize(members)>16)return fail(error,"Group limit: 16 unique members");
  cJSON_ArrayForEach(member,members){
   if(!reference(r,"devices",member,false))return fail(error,"Group references missing device");
   for(const cJSON *other=member->next;other;other=other->next)if(cJSON_IsNumber(other)&&other->valuedouble==member->valuedouble)return fail(error,"Duplicate group member");
  }
 }
 cJSON_ArrayForEach(v,get(r,"dashboards")){
  const cJSON *tiles=get(v,"tiles"),*t;if(!keys(v,"|id||name||tiles|")||!cJSON_IsArray(tiles)||cJSON_GetArraySize(tiles)>DSR_CONFIG_MAX_TILES)return fail(error,"Dashboard tile limit");
  cJSON_ArrayForEach(t,tiles){
   if(!keys(t,"|id||type||ref||span||height||fields|")||!number(t,"id",1,1000000)||!choice(t,"type","|device||room||scene||group|")||!number(t,"span",1,2)||!number(t,"height",1,2)||!number(t,"fields",0,255))return fail(error,"Invalid tile");
   const char *type=get(t,"type")->valuestring;
   if(strcmp(type,"room")&&get(t,"span")->valueint!=2)return fail(error,"Device and scene cards must use full width");
   if(!reference(r,!strcmp(type,"device")?"devices":!strcmp(type,"room")?"rooms":!strcmp(type,"group")?"groups":"scenes",get(t,"ref"),false))return fail(error,"Tile references missing object");
   for(const cJSON *b=t->next;b;b=b->next)if(number(b,"id",1,1000000)&&get(b,"id")->valueint==get(t,"id")->valueint)return fail(error,"Duplicate tile ID");
  }
 }
 error[0]=0;return true;
}
