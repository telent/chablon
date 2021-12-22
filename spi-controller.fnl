(local buffer-length 10)

(fn copy-to-byte-buffer [buffer tbl]
  (assert (< (# tbl) buffer-length)
          "payload too long to convert from table")
  (each [i v (ipairs tbl)]
    (tset buffer i v))
  buffer)

(fn new-spi [instance params]
  (let [buffer (byte_buffer.new buffer-length)
        handle (spictl_ffi.new instance params)]
    {
     :_buffer buffer
     :_handle handle
     :transfer (fn [spi payload count]
                 (let [buf (copy-to-byte-buffer spi._buffer payload)
                       len (or count (# payload)) ]
                   (spictl_ffi.transfer spi._handle buf len)
                   ))
     :transfer_raw (fn [spi payload count]
                     (spictl_ffi.transfer spi._handle payload count))
     }))

{
 :new new-spi
 }
