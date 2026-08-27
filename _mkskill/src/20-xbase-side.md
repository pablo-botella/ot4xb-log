---
mkskill:
  pos: 20
  in: "*"
---
## Sending lines from Xbase++

The sender lives in **ot4xb.dll**, the author's Xbase++ library. Among many
other things it exports two functions that push a string to `ot4xb_log.exe`:

```xbase
ot4xb_lSendLogStrFL( <cFunc>, <nLine>, <cFormat> [, <params, ...> ] )
ot4xb_lSendLogStr( <cFormat> [, <params, ...> ] )
```

`...FL` is the *func/line* flavour: it takes the calling function and the source
line as the first two arguments, so the entry arrives already tagged with where
it came from. Both format the remaining arguments and deliver a single string,
and both return `.T.` when the message reached its destination - which is also
the way to ask, from the program, whether anybody is listening.

What they do with it, as far as the viewer is concerned:

- they look for the viewer window by its class UUID
  (`11CBDBE2_0AF0_4713_B463_269FA6E2654B`) and, if found, deliver the text with
  `WM_COPYDATA`;
- if the viewer is **not running they do nothing, silently**: they just return
  `.F.` - no error, no file, no attempt to launch anything. Nothing ever
  launches `ot4xb_log.exe` automatically - you start it by hand when you want
  to look.

### An example: a pair of trace commands

Those two functions are the raw material; how each application calls them is
its own business. As an example, this is a pair of `#xcommand` definitions the
author uses in his own programs - copy them, or write whatever fits your code:

```xbase
#ifdef TRACE_ENABLED
#xcommand TRACEX <p1> [,<pN>] => ot4xb_lSendLogStrFL( ProcName() , __LINE__  ,"%s", "" + Var2Char(<p1>) [ + Var2Char(<pN>)] )
#xcommand TRACE  <p1> [,<pN>] => ot4xb_lSendLogStrFL( ProcName() , __LINE__  , cPrintf(, <p1> [ , <pN>] ) )
#else
#xcommand TRACEX <p1> [,<pN>] =>
#xcommand TRACE  <p1> [,<pN>] =>
#endif
```

- `TRACE cFormat [, args...]` - `cPrintf`-style: `TRACE "n=%d name=%s", n, cName`.
- `TRACEX expr [, exprN...]` - each argument goes through `Var2Char()` and the
  results are concatenated: `TRACEX "value: ", nValue, " / ", oObj`.
- Both pass `ProcName()` and `__LINE__`, so every entry carries the calling
  function and source line.
- With `TRACE_ENABLED` undefined the commands expand to **nothing**: the
  production build carries no calls at all, without touching a single line of
  code.

### An instance of your own

By default every application that uses `ot4xb_log.exe` writes to the **same
output device**: one viewer, one `.log`, everybody's lines mixed together. For
occasional messages and for debugging that is perfect - you launch one viewer
and see whatever happens. But to monitor one application in detail it becomes
unusable: your lines drown among everyone else's.

For that case ot4xb has a second group of functions:

```xbase
register_user_log_uuid( <cUserWndClassName> )
lSendLogStr( <cFormat> [, <params, ...> ] )
lSendLogStrFl( <cFile>, <nLine>, <cFormat> [, <params, ...> ] )
```

The prefix tells the two groups apart, and the difference is which identifier
they aim at:

- the `ot4xb_`-prefixed functions **always** use ot4xb's own UUID - the shared
  device described above;
- the ones without the prefix use **the UUID assigned at application level**,
  the one you set with `register_user_log_uuid`.

Register the class name once and from then on `lSendLogStr` / `lSendLogStrFl`
go only there. Then start a second instance of the viewer with that same class
name:

```
ot4xb_log.exe --class MYAPP_LOG_CLASS --title "My application" --log myapp --icon myapp.ico
```

That instance receives only your application's lines and keeps them in its own
`myapp.log`, while the default viewer, if it is running, carries on with
everybody else's. Its own title and icon make it recognisable in the tray when
several are open at once. The class name is what separates the instances (see
*Single instance*), so pick something unlikely to collide - a UUID is the
obvious choice.

### Two ways to switch logging on

1. **Compile time** - define `TRACE_ENABLED` (or whatever your own commands
   test) while developing and testing, and leave it undefined for the
   production build, as above.
2. **Run time** - keep the calls compiled but route them through a small
   function of your own that only calls `ot4xb_lSendLogStr...` when a program
   flag (command-line switch, ini entry, variable...) is set. This is the
   recommended shape for production: logging can be enabled and disabled
   without recompiling. Leaving the calls always active is also possible
   (without the viewer they make no noise), but that is each application's
   decision.

### The typical scenario

While developing you keep `ot4xb_log.exe` open and watch the lines arrive.
On a customer's machine the application runs as usual and the viewer is not
there. When something goes wrong and you need extra information, you launch
`ot4xb_log.exe` (a ~110 KB single file, no installation, no dependencies):
the DLL finds the window by its class UUID and from that moment every line
shows up in the viewer and is kept in the `.log`, with no recompile and no
restart of the application. Close the viewer and silence returns.
