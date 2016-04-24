module Washi
  class Tiler
    def self.call(input_path, output_path, options={})
      unless tile_size = options[:tile_size]
        fail "Need pass :tile_size"
      end
      `vips dzsave #{input_path} #{output_path} --tile-size #{tile_size} --overlap 0 --vips-progress --vips-leak`
    end
  end
end
