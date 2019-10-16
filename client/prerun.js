var Module = typeof Module !== 'undefined' ? Module : {};

if (!Module.preRun') Module.preRun = [];
Module.preRun.push(
  function() {
    FS.mkdir('/home/web_user/.freeciv');
    FS.mount(IDBFS, {}, '/home/web_user/.freeciv');
    FS.syncfs(true, function(err) {});
  }
)
