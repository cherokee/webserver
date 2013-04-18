import CTK

TABLE = [
    ('Primero', 'First',  '1st'),
    ('Segundo', 'Second', '2nd'),
    ('Tercero', 'Third',  '3rd'),
    ('Cuarto',  'Fourth', '4th'),
    ('Quito',   'Fifth',  '5th'),
    ('Sexto',   'Sixth',  '6th')
]

def table_reordered (post_key):
    print "New Order", CTK.post[post_key].split(',')
    return "{'ret': 'ok'}"

def default():
    table = CTK.SortableList (table_reordered)

    # Header
    table += [CTK.RawHTML("Spanish"), CTK.RawHTML("English")]
    table.set_header(1)

    # Body
    for n in range(len(TABLE)):
        table += [CTK.RawHTML(x) for x in TABLE[n]]
        table[-1].props['id'] = str(n+1)

    # Page
    page = CTK.Page ()
    page += CTK.RawHTML("<h1>Sortable List</h1>")
    page += table
    return page.Render()

CTK.publish ('', default)
CTK.run (port=8000)
