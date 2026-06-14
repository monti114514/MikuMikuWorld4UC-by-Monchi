def app_version_parts
  header = File.read("./MikuMikuWorld/Version.h")
  %w[MAJOR MINOR PATCH BUILD].map do |part|
    header.match(/#define\s+MMW_APP_VERSION_#{part}\s+(\d+)/)&.[](1) ||
      abort("MMW_APP_VERSION_#{part} not found")
  end
end

def app_version
  app_version_parts[0, 3].join(".")
end

def windows_version
  app_version_parts.join(".")
end

def update_version_define(header, name, value)
  pattern = /#define\s+#{name}\s+\d+/
  abort "#{name} not found" unless header.match?(pattern)
  header.gsub(pattern, "#define #{name} #{value}")
end

task "build:msbuild" do
  puts "Running msbuild"
  sh "msbuild /p:Configuration=Release /p:Platform=x64"
end

task "build:copy" do
  require "fileutils"

  puts "Building build/"

  FileUtils.rm_rf "build"
  FileUtils.mkdir_p "build"
  FileUtils.cp_r "x64/Release", "build/MikuMikuWorld"
  FileUtils.cp "build/MikuMikuWorld/MikuMikuWorld.pdb",
               "build/__YOU_DONT_NEED_THIS__.pdb"
  FileUtils.rm "build/MikuMikuWorld/MikuMikuWorld.pdb"
end

task "build:installer" do
  puts "Building installer"
  installer = File.read("./installerBase.nsi")
  File.write(
    "./installer.nsi",
    installer
      .gsub(/{version}/, app_version)
      .gsub(/{windows_version}/, windows_version)
  )

  candidates = []
  exts = ENV["PATHEXT"]&.split(";") || [""]
  ENV["PATH"].to_s.split(File::PATH_SEPARATOR).each do |dir|
    exts.each do |ext|
      candidates << File.join(dir, "makensis#{ext}")
    end
  end

  candidates << "C:/Program Files (x86)/NSIS/makensis.exe"
  candidates << "C:/Program Files/NSIS/makensis.exe"

  makensis = candidates.find { |p| File.file?(p) }

  abort "makensis not found" unless makensis

  sh %("#{makensis}" installer.nsi)
end

task "build:zip" do
  puts "Building zip"
  sh "7z a -tzip MikuMikuWorld.zip MikuMikuWorld", { chdir: "build" }
end

task "build" => %w[build:msbuild build:copy build:installer build:zip]

task "check:translation" do
  def list_keys(file)
    file
      .split("\n")
      .filter_map do |line|
        next nil if line.start_with?("#")
        next nil unless line.include?(",")
        line.split(",")[0]
      end
  end

  files = Dir.children("./MikuMikuWorld/res/i18n")
  template = File.read("./MikuMikuWorld/res/i18n/.template.csv")
  template_keys = list_keys(template)

  coverages = {}
  files.each do |file|
    next if file == ".template.csv"

    puts "== #{file}"
    keys = list_keys(File.read("./MikuMikuWorld/res/i18n/#{file}"))
    missing_keys = template_keys - keys
    if missing_keys.size > 0
      puts "missing keys:"
      puts "  " + missing_keys.join(", ")
    end
    coverage =
      ((1 - missing_keys.size.to_f / template_keys.size) * 100).round(2)
    puts "Coverage: #{coverage}%"
    coverages[file.split(".")[0]] = coverage
  end
  if (output_file = ENV["GITHUB_OUTPUT"])
    puts "Writing to GITHUB_OUTPUT"
    pp coverages
    coverages.each do |lang, coverage|
      File.write(output_file, "coverage_#{lang}=#{coverage}\n", mode: "a+")
    end
  end
end

task "check" => %w[check:translation]

task "update", [:version] do |t, args|
  abort "version is required" unless args[:version]

  version_raw = args[:version].sub(/\Av/i, "")
  version = version_raw.sub(/[^0-9.].*/, "").split(".")
  abort "version must be MAJOR.MINOR.PATCH[.BUILD]" unless (3..4).cover?(version.size)
  abort "version must be numeric" unless version.all? { |part| part.match?(/\A\d+\z/) }
  version << "0" while version.size < 4

  puts "Update: v#{version_raw} (#{version.join(".")})"
  header = File.read("./MikuMikuWorld/Version.h")
  %w[MAJOR MINOR PATCH BUILD].each_with_index do |part, index|
    header = update_version_define(header, "MMW_APP_VERSION_#{part}", version[index])
  end
  File.write("./MikuMikuWorld/Version.h", header)

  changelog_path = ".update-changelog"
  changelog = File.exist?(changelog_path) ? File.read(changelog_path).strip : ""

  commit_message = if changelog.empty?
    "release: v#{version_raw}"
  else
    <<~MSG.strip
      release: v#{version_raw}

      #{changelog}
    MSG
  end

  sh %Q(git commit --allow-empty -am "#{commit_message.gsub('"', '\"')}")
  sh %Q(git tag -f v#{version_raw})
end

task "action:version" do
  require "open3"

  ref = ENV["GITHUB_REF"] or abort "GITHUB_REF not set"
  tag = ref.split("/").last

  version_raw = app_version

  if tag.include?("-preview")
    body, status = Open3.capture2("git", "log", "-1", "--pretty=%b")
    abort "git log failed" unless status.success?

    prerelease = "true"
    release_body = body.strip
  else
    tags, status = Open3.capture2("git", "tag", "--sort=-creatordate")
    abort "git tag failed" unless status.success?

    prev_tag = tags
    .lines
    .map(&:strip)
    .reject { |t| t.start_with?(tag) || t.match?("v.+-.+") }
    .first or ""

    prerelease = "false"
    release_body = <<~BODY
      Download "mmw4uc-#{version_raw}-setup.exe" or "MikuMikuWorld.zip".

      Full Changelog: #{prev_tag}..#{tag}
    BODY
  end

  github_output = ENV["GITHUB_OUTPUT"] or abort "GITHUB_OUTPUT not set"

  File.open(github_output, "a") do |f|
    f.puts "prerelease=#{prerelease}"
    f.puts "body<<EOF"
    f.puts release_body
    f.puts "EOF"
  end
end
