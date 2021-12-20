(local buffer-length 10)

(fn coerce-to-byte-buffer [buffer tbl]
  (if (= (getmetatable tbl) "byte_buffer")
      in
      (do
        (assert (< (# tbl) buffer-length)
                "payload too long to convert from table")
        (each [i v (ipairs tbl)]
          (tset buffer i v))
        buffer)))

(fn new-spi [instance params]
  (let [buffer (byte_buffer.new buffer-length)
        handle (spictl_ffi.new instance params)]
    {
     :transfer (fn [spi bytes count]
                 (let [buf  (coerce-to-byte-buffer buffer bytes)
                       len (or count (# bytes)) ]
                   (spictl_ffi.transfer handle buf len)
                   ))
     }))

{
 :new new-spi
 }
