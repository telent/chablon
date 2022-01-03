(local json (require "json"))

(fn from-json [filename]
  (json.decode (: (_G.io.open filename "r") :read)))

(fn r. [tbl ...]
  (let [[fst & more] [...]]
    (if (next more)
        (r. (. tbl fst) (table.unpack more))
        (. tbl fst))))

(fn offset [file tag member]
  (let [j (from-json file)]
    (r. j :tag tag :members member :offset)))

{ : offset }
