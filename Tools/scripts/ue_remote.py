# Send Python to a running Unreal Editor via remote execution.
# usage: python ue_remote.py <file.py>   (or "-c" "<code>")
import sys, time
sys.path.insert(0, r"D:\Epic Games\UE_5.4\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python")
import remote_execution as re

if sys.argv[1] != "-c":
    # send a short command that runs the file inside the editor (big scripts overflow the reply buffer)
    import os
    p = os.path.abspath(sys.argv[1]).replace("\\", "/")
    LOG = os.path.join(os.environ.get("TEMP", "."), "ue_remote_out.txt").replace("\\", "/")
    if os.path.exists(LOG):
        os.remove(LOG)
    # capture the script's prints into a file (long output overflows the remote-execution reply)
    code = (
        "import sys, io, traceback\n"
        "_fv_buf = io.StringIO(); _fv_old = sys.stdout; sys.stdout = _fv_buf\n"
        "try:\n"
        f"    exec(compile(open(r'{p}', encoding='utf-8-sig').read(), r'{p}', 'exec'), {{'__name__': '__main__'}})\n"
        "except Exception:\n"
        "    _fv_buf.write(traceback.format_exc()); raise\n"
        "finally:\n"
        "    sys.stdout = _fv_old\n"
        f"    open(r'{LOG}', 'w', encoding='utf-8').write(_fv_buf.getvalue())\n"
    )
else:
    code = sys.argv[2]
cfg = re.RemoteExecutionConfig()
cfg.multicast_bind_address = "127.0.0.1"
rx = re.RemoteExecution(cfg)
rx.start()
node = None
for _ in range(50):
    if rx.remote_nodes:
        node = rx.remote_nodes[0]["node_id"]; break
    time.sleep(0.2)
if not node:
    print("NO_EDITOR_FOUND"); rx.stop(); sys.exit(2)
rx.open_command_connection(node)
res = rx.run_command(code, unattended=True, exec_mode=re.MODE_EXEC_FILE)
for o in res.get("output", []):
    print(o.get("output", "").rstrip())
if sys.argv[1] != "-c" and os.path.exists(LOG):
    print(open(LOG, encoding="utf-8").read().rstrip())
print("SUCCESS" if res.get("success") else "FAILED: " + str(res.get("result")))
rx.stop()
