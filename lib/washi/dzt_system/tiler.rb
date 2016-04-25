require 'mini_magick'

module MiniDzt
  class Tiler
    # Defaults
    DEFAULT_TILE_SIZE      = 512
    DEFAULT_TILE_OVERLAP   = 0
    DEFAULT_QUALITY        = 75
    DEFAULT_TILE_FORMAT    = 'jpg'
    DEFAULT_OVERWRITE_FLAG = false

    # Generates the DZI-formatted tiles and sets necessary metadata on this object.
    #
    # @param [Hash] options
    # @option source Magick::Image, or filename of image to be used for tiling
    # @option quality Image compression quality (default: 75)
    # @option format Format for output tiles (default: "jpg")
    # @option size Size, in pixels, for tile squares (default: 512)
    # @option overlap Size, in pixels, of the overlap between tiles (default: 2)
    # @option overwrite Whether or not to overwrite if the destination exists (default: false)
    def initialize(options)
      fail 'Missing options[:source].' unless options[:source]

      @tile_source  = MiniMagick::Image.open(options[:source])
      @tile_size    = options[:size]      || DEFAULT_TILE_SIZE
      @tile_overlap = options[:overlap]   || DEFAULT_TILE_OVERLAP
      @tile_format  = options[:format]    || DEFAULT_TILE_FORMAT
      @tile_quality = options[:quality]   || DEFAULT_QUALITY
      @overwrite    = options[:overwrite] || DEFAULT_OVERWRITE_FLAG

      @max_tiled_height = @tile_source.height
      @max_tiled_width = @tile_source.width
    end

    def slice!(output_dir)
      @tmp_working_dir = Pathname(@tile_source.path).dirname + "mini_dzt_#{SecureRandom.hex}"
      FileUtils.mkdir_p @tmp_working_dir

      orig_width = @tile_source.width
      orig_height = @tile_source.height

      max_level(orig_width, orig_height).downto(0) do |level|
        puts "Level #{level}..."
        current_level_storage_dir = "#{@tmp_working_dir}/#{level}"
        FileUtils.mkdir_p current_level_storage_dir

        width = @tile_source.width
        height = @tile_source.height

        manuscripts = get_manuscripts(width, height, current_level_storage_dir)

        manuscripts.each do |dest_path, geometry_arg|
          crope_image(@tile_source, dest_path, geometry_arg, @tile_quality)
        end

        @tile_source.resize("50%")
      end
      FileUtils.mv @tmp_working_dir, output_dir
    end

    private
      def tile_counts(length)
        return 1 if length <= @tile_size
        (length / @tile_size.to_f).ceil
      end

      def get_manuscripts(width, height, current_level_storage_dir)
        manuscripts = []

        overlapping_tile_size = @tile_size + @tile_overlap
        col_counts = tile_counts(width)
        row_counts = tile_counts(height)
        x, y = [0, 0]

        row_counts.times do |row_count|
          col_counts.times do |col_count|
            manuscripts << [
              File.join(current_level_storage_dir, "#{col_count}_#{row_count}.#{@tile_format}"),
              "#{overlapping_tile_size}x#{overlapping_tile_size}+#{x}+#{y}"
            ]
            x += @tile_size
          end
          x = 0
          y += @tile_size
        end
        manuscripts
      end

      def max_level(width, height)
        (Math.log([width, height].max) / Math.log(2)).ceil
      end

      def crope_image(image, dest_path, geometry_arg, quality = 75)
        puts geometry_arg
        cmd = "convert -crop #{geometry_arg} #{image.path} #{dest_path}"
        system(cmd)
      end
  end
end
