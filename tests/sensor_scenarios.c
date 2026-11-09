#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)
__declspec(dllexport) int run_sensor_tests(void) {
    CHECK(sgp30_init() == 0);
    CHECK(sgp30_warmup_remaining_sec() == 15);
    CHECK(!sgp30_values_ready());
    CHECK(!sgp30_data.baseline_restored);
    CHECK(sgp30_read() == 0);
    CHECK(!sgp30_values_ready());
    fake_us = sgp30_data.started_us + 15000000;
    CHECK(!sgp30_values_ready()); // Warmup over, but no post-warmup sample yet.
    CHECK(sgp30_read() == 0);
    CHECK(sgp30_values_ready() && sgp30_is_learning());
    CHECK(get_co2_sgp30() == 800 && get_tvoc_sgp30() == 20);
    fake_eco2 = 1200; fake_tvoc = 40; fake_us += 1000000;
    CHECK(sgp30_read() == 0);
    CHECK(get_co2_sgp30() == 1000 && get_tvoc_sgp30() == 30);
    for (int i = 0; i < 100; i++) CHECK(get_co2_sgp30() == 1000);
    fake_crc_error = 1;
    CHECK(sgp30_read() != 0 && !sgp30_values_ready());
    fake_crc_error = 0; fake_io_error = 1;
    CHECK(sgp30_read() != 0 && !sgp30_values_ready());
    fake_io_error = 0;
    CHECK(sgp30_read() == 0 && sgp30_values_ready());
    fake_us += 3000000;
    CHECK(!sgp30_values_ready());
    CHECK(sgp30_read() == 0 && sgp30_values_ready());
    CHECK(fake_nvs_commits == 0);
    // Accelerate 12 hours on the fake clock, executing the production read path.
    while (sgp30_learning_remaining_sec()) {
        fake_us += 1000000;
        CHECK(sgp30_read() == 0);
    }
    CHECK(sgp30_data.baseline_ready && sgp30_data.baseline_saved);
    CHECK(fake_nvs_commits == 1);
    fake_us += 3599000000LL;
    CHECK(sgp30_read() == 0 && fake_nvs_commits == 1);
    fake_us += 1000000;
    CHECK(sgp30_read() == 0 && fake_nvs_commits == 2);
    fake_epoch += 100;
    CHECK(sgp30_init() == 0 && sgp30_data.baseline_restored);
    CHECK(fake_set_baseline[2] == 0x45 && fake_set_baseline[3] == 0x67);
    CHECK(fake_set_baseline[5] == 0xab && fake_set_baseline[6] == 0xcd);
    CHECK(fake_set_baseline[4] == fake_crc(0x4567));
    CHECK(fake_set_baseline[7] == fake_crc(0xabcd));
    fake_serial++;
    CHECK(sgp30_init() == 0 && !sgp30_data.baseline_restored);
    fake_serial--;
    fake_epoch = 1800000000 + SGP30_BASELINE_MAX_AGE_SEC;
    CHECK(sgp30_init() == 0 && !sgp30_data.baseline_restored);
    fake_epoch = 0;
    CHECK(sgp30_init() == 0 && !sgp30_data.baseline_restored);
    fake_epoch = 1800000100;
    CHECK(sgp30_read() == 0 && sgp30_data.baseline_restored); // Late SNTP sync.
    fake_epoch = 1799999999;
    CHECK(sgp30_init() == 0 && !sgp30_data.baseline_restored); // Future-dated record.
    fake_io_error = 1;
    CHECK(sgp30_init() != 0 && !sgp30_data.init_ok && !sgp30_values_ready());
    return 0;
}
