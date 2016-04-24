require 'test_helper'

class WashiTest < Minitest::Test
  def setup
    @sample_img1 = File.join File.expand_path('../fixtures', __FILE__), "img1.jpg"
    @sample_img2 = File.join File.expand_path('../fixtures', __FILE__), "img2.jpg"
    @sample_img3 = File.join File.expand_path('../fixtures', __FILE__), "img3.jpg"

    @output_path = File.expand_path('../tmp', __FILE__)
    @output_dir  = File.expand_path('../tmp_files', __FILE__)
  end

  def after_teardown
    FileUtils.rm_rf(@output_dir)
    FileUtils.rm_rf(@output_path + ".dzi")
  end

  def test_img1
    Washi::Tiler.call @sample_img1, @output_path, tile_size: 512

    assert_equal 14, Dir.glob(@output_dir + "/*").count
    assert_equal 9 * 4, Dir.glob(@output_dir + "/13/*").count
    assert_equal 61, Dir.glob(@output_dir + "/*/*").count
    Dir.glob(@output_dir + "/13/*").each do |img_path|
      img = MiniMagick::Image.open img_path
      assert(img.width > 200)
      assert(img.height > 400)
    end
  end

  def test_img2
    Washi::Tiler.call @sample_img2, @output_path, tile_size: 512

    assert_equal 14, Dir.glob(@output_dir + "/*").count
    assert_equal 16 * 11, Dir.glob(@output_dir + "/13/*").count
    Dir.glob(@output_dir + "/13/*").each do |img_path|
      img = MiniMagick::Image.open img_path
      assert(img.width > 100)
      assert(img.height > 100)
    end
  end

  def test_that_it_has_a_version_number
    refute_nil ::Washi::VERSION
  end
end
