(fn set-from-table [buffer tbl]
  (each [i v (ipairs tbl)]
    (tset buffer i v))
  buffer)


{
 :new glue.byte_buffer_new
 :from_table set-from-table
 }
