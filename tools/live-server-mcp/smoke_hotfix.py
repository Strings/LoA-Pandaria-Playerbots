"""Opt-in T-045 hotfix regression using the repository headless client.
Requires a dedicated test account; creates Codextwink if the account has no characters.
Run after applying the T-045 migration and restarting the test worldserver.
"""
import importlib.util,sys,json,os,struct,time
from pathlib import Path
source=os.environ.get('MOP_HEADLESS_CLIENT',str(Path(__file__).resolve().parents[2]/'contrib/headless-client/headless.py'));spec=importlib.util.spec_from_file_location('headless',source);h=importlib.util.module_from_spec(spec);sys.modules['headless']=h;spec.loader.exec_module(h)
host=os.environ.get('MOP_HOST','127.0.0.1');account=os.environ['MOP_ACCOUNT'];password=os.environ['MOP_PASSWORD'];h.log=lambda *args: None;K=h.auth_login(host,int(os.environ.get('MOP_AUTH_PORT','3744')),account,password)
w=h.WorldClient(host,int(os.environ.get('MOP_WORLD_PORT','8105')),K,account);received=[];read=w.read_packet
def capture():
 op,body=read()
 if op in (0x103B,0x1EBA):received.append((op,body))
 return op,body
w.read_packet=capture
try:
 w.handshake(K);chars=w.char_enum()
 if not chars:
  w.char_create('Codextwink',2,1);w.pump(3);chars=w.char_enum()
 assert chars,'character create failed'
 assert w.player_login(chars[0]['guid'])
 w.pump(2)
 bw=h.BitWriter();bw.bits(1,21);bw.bits(0,8)
 w.send_packet(0x158D,struct.pack('<I',2442913102)+bw.flush()+struct.pack('<I',1482))
 w.pump(4,stop_on=(0x103B,))
 notices=[b for op,b in received if op==0x1EBA];replies=[b for op,b in received if op==0x103B]
 assert notices and replies,'missing hotfix notification/reply'
 notice=notices[-1];count=int.from_bytes(notice[:3],'big')>>4
 assert any(struct.unpack_from('<II',notice,3+n*12+4)==(1482,2442913102) for n in range(count)), 'Shadowfang absent from hotfix notice'
 body=replies[-1];entry,timestamp,kind,size=struct.unpack_from('<4I',body);assert entry==1482 and kind==2442913102 and len(body)==16+size
 sparse=body[16:];assert struct.unpack_from('<I',sparse,52)[0]==38
 assert struct.unpack_from('<I',sparse,104)[0]==32
 assert struct.unpack_from('<i',sparse,144)[0]==4
 print('PASS: authenticated headless login, hotfix notice, and Shadowfang Item-sparse reply (iLvl38; added crit rating4).')
finally:
 w.s.close()
