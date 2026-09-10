-- Demo for app.panel{} -- run with File > Scripts.
--
-- The dialog must stay alive for as long as the panel is docked: the panel
-- borrows the dialog's widgets, it does not copy them. Keeping it in a global
-- is the simplest way to do that from a script.

PanelDemoDialog = Dialog("Demo")
PanelDemoDialog
  :label{ text = "Docked, not floating." }
  :separator()
  :slider{ id = "size", label = "Size", min = 1, max = 64, value = 8 }
  :color{ id = "col", label = "Color", color = app.fgColor }
  :button{
    text = "Use it",
    onclick = function()
      local d = PanelDemoDialog.data
      app.fgColor = d.col
      app.command.BrushSize{ size = d.size }
    end,
  }
  :separator()
  :button{ text = "Close panel", onclick = function() app.closePanel("demo") end }

app.panel{ dialog = PanelDemoDialog, id = "demo", title = "Demo", side = "right" }
