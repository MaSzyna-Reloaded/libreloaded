extends ReloadedGutTest

## The caches kept within their size: the least recently used entries go first, an entry used in
## the session stays. The tests' own root, so nothing the game caches is measured or removed.

const TEST_ROOT:String = "user://test_resource_cache_server/"
const CACHE_DIRECTORY:String = "entries"
const MAX_SIZE_SETTING:String = "reloaded/cache/max_size"
const GIBIBYTE:int = 1024 * 1024 * 1024
## Real time between two entries written, so their last uses differ [ms]
const WRITE_INTERVAL_MSEC:int = 5

var _cache:ResourceCache
var _original_root:String
var _original_max_size:int


func before_each():
    # these tests are about the cache: on for their duration, where the tests' hook keeps it off
    ProjectSettings.set_setting(CACHE_ENABLED_SETTING, true)
    _original_root = ResourceCacheServer.root
    _original_max_size = ProjectSettings.get_setting(MAX_SIZE_SETTING)
    ResourceCacheServer.root = TEST_ROOT
    _cache = ResourceCache.create(CACHE_DIRECTORY)

func after_each():
    ResourceCacheServer.cache_clear("")
    ResourceCacheServer.root = _original_root
    ProjectSettings.set_setting(MAX_SIZE_SETTING, _original_max_size)
    ProjectSettings.set_setting(CACHE_ENABLED_SETTING, false)
    _cache = null


func test_the_budget_is_the_maximum_size_while_the_disk_has_room():
    assert_eq(ResourceCacheServer.budget_get(GIBIBYTE, 0, 100 * GIBIBYTE), GIBIBYTE)

func test_no_limit_leaves_only_the_free_space_reserve():
    # 1 GiB used and 10 GiB free: the caches may grow until 5 GiB are left free
    assert_eq(
        ResourceCacheServer.budget_get(ResourceCacheServer.MAX_SIZE_UNLIMITED, GIBIBYTE, 10 * GIBIBYTE),
        6 * GIBIBYTE
    )

func test_a_full_disk_shrinks_the_budget_under_the_maximum_size():
    assert_eq(ResourceCacheServer.budget_get(20 * GIBIBYTE, 2 * GIBIBYTE, 4 * GIBIBYTE), GIBIBYTE)
    assert_eq(ResourceCacheServer.budget_get(20 * GIBIBYTE, 0, GIBIBYTE), 0, "never less than nothing")

func test_the_choices_go_from_the_smallest_to_no_limit():
    var choices:PackedInt64Array = ResourceCacheServer.max_size_get_choices()
    assert_eq(choices[choices.size() - 1], ResourceCacheServer.MAX_SIZE_UNLIMITED)
    for index:int in range(1, choices.size() - 1):
        assert_gt(choices[index], choices[index - 1])

func test_the_default_maximum_size_is_a_limited_choice():
    var default_max_size:int = ProjectSettings.property_get_revert(MAX_SIZE_SETTING)
    assert_has(ResourceCacheServer.max_size_get_choices(), default_max_size)
    assert_ne(default_max_size, ResourceCacheServer.MAX_SIZE_UNLIMITED)
    assert_lte(default_max_size, 20 * GIBIBYTE)

func test_the_least_recently_used_entry_goes_first():
    _write("a.res")
    _write("b.res")
    _start_session()
    _trim_to(_entry_size("b.res"))
    assert_false(FileAccess.file_exists(_path("a.res")), "the older entry should be removed")
    assert_true(FileAccess.file_exists(_path("b.res")), "the newer entry should fit and stay")

func test_an_entry_used_in_the_session_stays():
    _write("a.res")
    _write("b.res")
    _start_session()
    assert_not_null(_cache.get("a.res"))
    _trim_to(_entry_size("a.res"))
    assert_true(FileAccess.file_exists(_path("a.res")), "the entry read in this session should stay")
    assert_false(FileAccess.file_exists(_path("b.res")), "the entry not used should be removed")

func test_the_last_use_outlives_the_session():
    _write("a.res")
    _write("b.res")
    _start_session()
    OS.delay_msec(WRITE_INTERVAL_MSEC)
    assert_not_null(_cache.get("a.res"))
    _start_session()
    _trim_to(_entry_size("a.res"))
    assert_true(FileAccess.file_exists(_path("a.res")), "the entry read last session should stay")
    assert_false(FileAccess.file_exists(_path("b.res")), "the entry written before it should go")

func test_an_entry_goes_with_its_hash_and_its_files_beside_it():
    _cache.set("c.res", Resource.new(), "hash")
    var beside:String = _cache.get_file_path("c")
    DirAccess.make_dir_recursive_absolute(beside)
    FileAccess.open(beside.path_join("chunk.bin"), FileAccess.WRITE).store_string("chunk")
    _start_session()
    # one byte: 0 would be no limit
    _trim_to(1)
    assert_false(FileAccess.file_exists(_path("c.res")))
    assert_false(FileAccess.file_exists(_path("c.res") + ".hash"))
    assert_false(DirAccess.dir_exists_absolute(beside))


func _write(path:String) -> void:
    _cache.set(path, Resource.new())
    OS.delay_msec(WRITE_INTERVAL_MSEC)

## Nothing written so far counts as used in it
func _start_session() -> void:
    ResourceCacheServer.root = TEST_ROOT

func _trim_to(max_size:int) -> void:
    ProjectSettings.set_setting(MAX_SIZE_SETTING, max_size)
    ResourceCacheServer.cache_trim()

## Not through the cache, which would mark the entry used
func _path(path:String) -> String:
    return TEST_ROOT.path_join(CACHE_DIRECTORY).path_join(path)

func _entry_size(path:String) -> int:
    return FileAccess.get_size(_path(path))
