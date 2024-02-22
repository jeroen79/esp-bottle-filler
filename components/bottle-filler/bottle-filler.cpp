/*
 * esp-bottle-filler
 * Copyright (C) Dekien Jeroen 2024
 */
#include "bottle-filler.h"

using namespace std;
using json = nlohmann::json;

static const char *TAG = "BottleFiller";

// esp http server only works with static handlers, no other option atm then to save a pointer.
BottleFiller *mainInstance;

BottleFiller::BottleFiller(SettingsManager *settingsManager)
{
	ESP_LOGI(TAG, "BottleFiller Construct");
	this->settingsManager = settingsManager;
	mainInstance = this;
}

void BottleFiller::Init()
{
	// read the post important settings first so when van set outputs asap.
	this->readSystemSettings();

	// get out fillers
	this->readFillerSettings();

	// init out fillers
	this->initFillers();

	// init gpio as soon as possible
	if (this->invertOutputs)
	{
		this->gpioHigh = 0;
		this->gpioLow = 1;
	}

	uint16_t freq = 5000; // Frequency in Hertz. Set frequency at 5 kHz

	// Prepare and then apply the LEDC PWM timer configuration
	ledc_timer_config_t ledc_timer = {};

	ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
	ledc_timer.timer_num = LEDC_TIMER_0;
	ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
	ledc_timer.freq_hz = freq; // Set output frequency at 5 kHz
	ledc_timer.clk_cfg = LEDC_AUTO_CLK;
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

	this->run = true;

	this->server = this->startWebserver();

	// init our inputs, this also starts the interrupt task
	this->initInputs();
}

void BottleFiller::readSystemSettings()
{
	ESP_LOGI(TAG, "Reading BottleFiller Settings");

	bool configInvertOutputs = false;
// is there a cleaner way to do this?, config to bool doesn't seem to work properly
#if defined(CONFIG_InvertOutputs)
	configInvertOutputs = true;
#endif
	this->invertOutputs = this->settingsManager->Read("invertOutputs", configInvertOutputs);

	ESP_LOGI(TAG, "Reading BottleFiller Settings Done");
}

void BottleFiller::saveSystemSettingsJson(json config)
{
	ESP_LOGI(TAG, "Saving System Settings");

	if (!config["invertOutputs"].is_null() && config["invertOutputs"].is_boolean())
	{
		this->settingsManager->Write("invertOutputs", (bool)config["invertOutputs"]);
		this->invertOutputs = (bool)config["invertOutputs"];
	}

	ESP_LOGI(TAG, "Saving System Settings Done");
}

void BottleFiller::startAutoFill(void *arg)
{
	//  we use our maininstance, passing 2 items via struct to xTaskCreate seems unreliable atm
	BottleFiller *instance = mainInstance;

	FillerConfig *filler = (FillerConfig *)arg;

	uint8_t fillerId = filler->id;

	if (fillerId < 1)
	{
		ESP_LOGE(TAG, "Filler id is invalid! %d", fillerId);
		vTaskDelete(NULL);
		return;
	}

	ESP_LOGI(TAG, "startAutoFill %d", fillerId);

	uint32_t ticks = xTaskGetTickCount();
	uint32_t stopAt = ticks + (filler->fillTime / 10);

	ESP_LOGI(TAG, "ticks %lu stopAt: %lu", ticks, stopAt);

	uint16_t fillSpeed = (instance->maxDuty / 100) * filler->autoFillSpeed;

	ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel, fillSpeed);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel);

	while (filler->status == Filling)
	{
		ticks = xTaskGetTickCount();

		ESP_LOGI(TAG, "ticks %lu stopAt: %lu", ticks, stopAt);

		if (ticks >= stopAt)
		{
			ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel, 0);
			ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel);

			ESP_LOGI(TAG, "Fill Complete %d", fillerId);
			break;
		}

		vTaskDelay(pdMS_TO_TICKS(100));
	}

	ESP_LOGI(TAG, "startAutoFill %d Done", fillerId);

	// reset status to idle
	filler->status = Idle;

	vTaskDelete(NULL);
}

void BottleFiller::start(uint8_t fillerId)
{
	ESP_LOGI(TAG, "Start ID:%d", fillerId);

	std::map<uint8_t, FillerConfig *>::iterator it;
	it = this->fillers.find(fillerId);

	if (it == this->fillers.end())
	{
		// doesn't exist anymore, just ignore
		ESP_LOGW(TAG, "Filler doesn't exist anymore, just ignore %d", fillerId);
		return;
	}

	FillerConfig *filler = it->second;
	if (filler->status == Idle)
	{
		filler->status = Filling;

		string taskName = "autofill" + to_string(fillerId);

		// we pass our pointer of our fillerconfig
		xTaskCreate(&this->startAutoFill, taskName.c_str(), 2048, filler, 5, NULL);
	}
	else
	{
		ESP_LOGI(TAG, "Not idle %d", filler->status);

		// stop at once
		ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel, 0);
		ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel);

		filler->status = Aborting;
	}
}

// via web fixed time
void BottleFiller::startManualFill(uint8_t fillerId, uint32_t time)
{
	std::map<uint8_t, FillerConfig *>::iterator it;
	it = this->fillers.find(fillerId);

	if (it == this->fillers.end())
	{
		// doesn't exist anymore, just ignore
		ESP_LOGW(TAG, "Filler doesn't exist anymore, just ignore %d", fillerId);
		return;
	}

	FillerConfig *filler = it->second;

	if (filler->status != Idle)
	{
		ESP_LOGW(TAG, "Filler must be idle %d", fillerId);
		return;
	}

	uint16_t fillSpeed = (this->maxDuty / 100) * filler->manualFillSpeed;

	ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel, fillSpeed);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel);

	vTaskDelay(pdMS_TO_TICKS(time));

	// stop
	ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel, 0);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel);
}

// for push button until release
void BottleFiller::startManualFill(uint8_t fillerId)
{
	std::map<uint8_t, FillerConfig *>::iterator it;
	it = this->fillers.find(fillerId);

	if (it == this->fillers.end())
	{
		// doesn't exist anymore, just ignore
		ESP_LOGW(TAG, "Filler doesn't exist anymore, just ignore %d", fillerId);
		return;
	}

	FillerConfig *filler = it->second;

	if (filler->status != Idle)
	{
		ESP_LOGW(TAG, "Filler must be idle %d", fillerId);
		return;
	}

	uint16_t fillSpeed = (this->maxDuty / 100) * filler->manualFillSpeed;

	ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel, fillSpeed);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel);
}

void BottleFiller::stopManualFill(uint8_t fillerId)
{
	std::map<uint8_t, FillerConfig *>::iterator it;
	it = this->fillers.find(fillerId);

	if (it == this->fillers.end())
	{
		// doesn't exist anymore, just ignore
		ESP_LOGW(TAG, "Filler doesn't exist anymore, just ignore %d", fillerId);
		return;
	}

	FillerConfig *filler = it->second;

	// stop
	ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel, 0);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)filler->channel);
}

string BottleFiller::bootIntoRecovery()
{
	// recovery is our factory
	esp_partition_subtype_t t = ESP_PARTITION_SUBTYPE_APP_FACTORY;
	const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, t, NULL);

	if (factory_partition == NULL)
	{
		ESP_LOGE(TAG, "Factory Partition not found!");
		return "Error: Factory Partition not found!";
	}

	if (esp_ota_set_boot_partition(factory_partition) == ESP_OK)
	{
		return "Device is booting into recovery hold on!";
	}

	return "Error: Unable to chage boot Partition!";
}

void BottleFiller::reboot(void *arg)
{
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	esp_restart();
}

void BottleFiller::addDefaultFillers()
{
	auto defaultFiller1 = new FillerConfig();
	defaultFiller1->id = 1;
	defaultFiller1->name = "Filler 1";
	defaultFiller1->pumpPin = (gpio_num_t)CONFIG_PUMP1;
	// defaultFiller1->autoPin = (gpio_num_t)39;
	// defaultFiller1->manualPin = (gpio_num_t)38;
	defaultFiller1->autoFillSpeed = 100;
	defaultFiller1->manualFillSpeed = 50;
	defaultFiller1->fillTime = 20000;
	this->fillers.insert_or_assign(defaultFiller1->id, defaultFiller1);

	auto defaultFiller2 = new FillerConfig();
	defaultFiller2->id = 2;
	defaultFiller2->name = "Filler 2";
	defaultFiller2->pumpPin = (gpio_num_t)CONFIG_PUMP2;
	// defaultFiller2->autoPin = (gpio_num_t)37;
	// defaultFiller2->manualPin = (gpio_num_t)36;
	defaultFiller2->autoFillSpeed = 100;
	defaultFiller2->manualFillSpeed = 50;
	defaultFiller2->fillTime = 20000;
	this->fillers.insert_or_assign(defaultFiller2->id, defaultFiller2);
}

void BottleFiller::readFillerSettings()
{
	vector<uint8_t> empty = json::to_msgpack(json::array({}));
	vector<uint8_t> serialized = this->settingsManager->Read("fillers", empty);

	json jFillers = json::from_msgpack(serialized);

	if (jFillers.empty())
	{
		ESP_LOGI(TAG, "Adding Default Fillers");
		this->addDefaultFillers();
	}
	else
	{

		for (auto &el : jFillers.items())
		{
			auto jFiller = el.value();

			auto filler = new FillerConfig();
			filler->from_json(jFiller);

			uint8_t fillerId = filler->id;

			ESP_LOGI(TAG, "Filler From Settings ID:%d", fillerId);

			this->fillers.insert_or_assign(fillerId, filler);
		}
	}
}

void BottleFiller::setFillerSettings(json jFiller)
{
	ESP_LOGI(TAG, "Setting Filler Settings");

	if (!jFiller.is_object())
	{
		ESP_LOGW(TAG, "Filler settings must be an object!");
		return;
	}

	uint8_t fillerId = jFiller["id"];

	std::map<uint8_t, FillerConfig *>::iterator it;
	it = this->fillers.find(fillerId);

	if (it == this->fillers.end())
	{
		// doesn't exist anymore, just ignore
		ESP_LOGW(TAG, "Filler doesn't exist anymore, just ignore %d", fillerId);
		return;
	}

	FillerConfig *filler = it->second;

	if (!jFiller["autoFillSpeed"].is_null() && jFiller["autoFillSpeed"].is_number())
	{
		filler->autoFillSpeed = jFiller["autoFillSpeed"].get<int>();
	}

	if (!jFiller["manualFillSpeed"].is_null() && jFiller["manualFillSpeed"].is_number())
	{
		filler->manualFillSpeed = jFiller["manualFillSpeed"].get<int>();
	}

	if (!jFiller["fillTime"].is_null() && jFiller["fillTime"].is_number())
	{
		filler->fillTime = jFiller["fillTime"].get<int>();
	}

	ESP_LOGI(TAG, "Done Setting Filler Settings");
}

void BottleFiller::saveFillerSettings(json jFillers)
{
	ESP_LOGI(TAG, "Saving Filler Settings");

	if (!jFillers.is_array())
	{
		ESP_LOGW(TAG, "Filler settings must be an array!");
		return;
	}

	// put everything on hold
	this->run = false;
	this->interruptRun = false;

	// wait for stop
	vTaskDelay(pdMS_TO_TICKS(1000));

	// clear
	this->fillers.clear();
	this->inputs.clear();

	uint8_t newId = 0;

	// update running data
	for (auto &el : jFillers.items())
	{
		newId++;

		if (newId > 6)
		{
			ESP_LOGE(TAG, "Only 6 Fillers supported!");
			continue;
		}

		auto jFiller = el.value();
		uint8_t fillerId = newId;
		jFiller["id"] = fillerId;

		auto filler = new FillerConfig();
		filler->from_json(jFiller);
		filler->id = fillerId;

		this->fillers.insert_or_assign(fillerId, filler);
	}

	// Serialize to MessagePack for size
	vector<uint8_t> serialized = json::to_msgpack(jFillers);

	this->settingsManager->Write("fillers", serialized);

	this->initFillers();
	this->run = true;

	this->initInputs();

	ESP_LOGI(TAG, "Saving Filler Settings Done");
}

void BottleFiller::initFillers()
{

	uint8_t nextInputId = 0;
	// link our channel to our fillers, after loading or saving changes
	for (auto const &[key, filler] : this->fillers)
	{
		uint8_t channelId = filler->id - 1;
		filler->channel = channelId;

		ESP_LOGI(TAG, "Initilizing Filler:%d on Channel:%d", filler->id, channelId);

		ledc_channel_config_t ledc_channel = {};
		ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
		ledc_channel.channel = (ledc_channel_t)channelId;
		ledc_channel.timer_sel = LEDC_TIMER_0;
		ledc_channel.intr_type = LEDC_INTR_DISABLE;
		ledc_channel.gpio_num = filler->pumpPin;
		ledc_channel.duty = 0; // Set duty to 0
		ledc_channel.hpoint = 0;
		ledc_channel.flags.output_invert = 0;
		ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

		if (filler->autoPin > 0)
		{
			nextInputId++;
			Input newInput;
			newInput.Id = nextInputId;
			newInput.GpioNr = (gpio_num_t)filler->autoPin;
			newInput.FillerId = filler->id;
			newInput.CurrentLevel = 1;
			newInput.Function = AutoFill;
			inputs.push_back(newInput); // add to map
		}

		if (filler->manualPin > 0)
		{
			nextInputId++;
			Input newInput;
			newInput.Id = nextInputId;
			newInput.GpioNr = (gpio_num_t)filler->manualPin;
			newInput.FillerId = filler->id;
			newInput.CurrentLevel = 1;
			newInput.Function = ManualFill;
			inputs.push_back(newInput); // add to map
		}
	}
}

void BottleFiller::initInputs()
{

	if (!inputs.empty())
	{

		for (const auto &input : inputs)
		{
			ESP_LOGI(TAG, "Setting %d to input", input.GpioNr);

			gpio_config_t io_conf = {};
			io_conf.mode = GPIO_MODE_INPUT;
			io_conf.pin_bit_mask = (1ULL << input.GpioNr);
			io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
			gpio_config(&io_conf);
		}

		// start gpio task
		this->interruptRun = true;
		xTaskCreate(&this->interruptLoop, "interruptLoop_task", 2048, this, 10, NULL);
	}
}

void BottleFiller::interruptLoop(void *arg)
{
	BottleFiller *instance = (BottleFiller *)arg;
	// gpio_num_t gpioNr;

	while (instance->run && instance->interruptRun)
	{
		vTaskDelay(pdMS_TO_TICKS(100));

		for (Input &input : instance->inputs)
		{
			uint32_t timestamp = xTaskGetTickCount() / 100; // we only need whole seconds
			uint8_t level = gpio_get_level((gpio_num_t)input.GpioNr);

			if (input.CurrentLevel != level)
			{
				if (level > 0)
				{
					uint32_t elapsedTime = timestamp - input.LastChange;

					ESP_LOGI(TAG, "Released %d %d Time:%lu", input.FillerId, input.Function, elapsedTime);

					if (input.Function == AutoFill)
					{
						instance->start(input.FillerId);
					}

					else if (input.Function == ManualFill)
					{
						instance->stopManualFill(input.FillerId);
					}
				}
				else
				{
					// high to low
					input.LastChange = timestamp;
					ESP_LOGI(TAG, "Pressed %d %d", input.FillerId, input.Function);

					if (input.Function == ManualFill)
					{
						instance->startManualFill(input.FillerId);
					}
				}

				input.CurrentLevel = level;
			}
		}
	}

	vTaskDelete(NULL);
}

string BottleFiller::processCommand(string payLoad)
{
	ESP_LOGD(TAG, "payLoad %s", payLoad.c_str());

	json jCommand = json::parse(payLoad);
	string command = jCommand["command"];
	json data = jCommand["data"];

	ESP_LOGD(TAG, "processCommand %s", command.c_str());
	ESP_LOGD(TAG, "data %s", data.dump().c_str());

	json resultData = {};
	string message = "";
	bool success = true;

	if (command == "Start")
	{
		uint8_t id = data["id"].get<uint>();
		this->start(id);
	}
	else if (command == "StartManual")
	{
		uint8_t id = data["id"].get<uint>();
		uint32_t time = data["time"].get<uint>(); // ms
		this->startManualFill(id, time);
	}
	else if (command == "GetWifiSettings")
	{
		// get data from wifi-connect
		if (this->GetWifiSettingsJson)
		{
			resultData = this->GetWifiSettingsJson();
		}
	}
	else if (command == "SaveWifiSettings")
	{
		// save via wifi-connect
		if (this->SaveWifiSettingsJson)
		{
			this->SaveWifiSettingsJson(data);
		}
		message = "Please restart device for changes to have effect!";
	}
	else if (command == "ScanWifi")
	{
		// scans for networks
		if (this->ScanWifiJson)
		{
			resultData = this->ScanWifiJson();
		}
	}
	else if (command == "GetSystemSettings")
	{
		resultData = {
			{"invertOutputs", this->invertOutputs}};
	}
	else if (command == "SaveSystemSettings")
	{
		this->saveSystemSettingsJson(data);
		message = "Please restart device for changes to have effect!";
	}
	else if (command == "GetFillerSettings")
	{
		// Convert sensors to json
		json jFillers = json::array({});

		for (auto const &[key, val] : this->fillers)
		{
			json jFiller = val->to_json();
			jFillers.push_back(jFiller);
		}

		resultData = jFillers;
	}
	else if (command == "SaveFillerSettings")
	{
		this->saveFillerSettings(data);
	}
	else if (command == "SetFillerSettings")
	{
		this->setFillerSettings(data);
	}
	else if (command == "Reboot")
	{
		xTaskCreate(&this->reboot, "reboot_task", 1024, this, 5, NULL);
	}
	else if (command == "FactoryReset")
	{
		this->settingsManager->FactoryReset();
		message = "Device will restart shortly, reconnect to factory wifi settings to continue!";
		xTaskCreate(&this->reboot, "reboot_task", 1024, this, 5, NULL);
	}
	else if (command == "BootIntoRecovery")
	{
		message = this->bootIntoRecovery();

		if (message.find("Error") != std::string::npos)
		{
			success = false;
		}
		else
		{
			xTaskCreate(&this->reboot, "reboot_task", 1024, this, 5, NULL);
		}
	}

	json jResultPayload;
	jResultPayload["data"] = resultData;
	jResultPayload["success"] = success;

	if (message != "")
	{
		jResultPayload["message"] = message;
	}

	string resultPayload = jResultPayload.dump();

	return resultPayload;
}

httpd_handle_t BottleFiller::startWebserver(void)
{

	httpd_uri_t indexUri;
	indexUri.uri = "/";
	indexUri.method = HTTP_GET;
	indexUri.handler = this->indexGetHandler;

	httpd_uri_t logoUri;
	logoUri.uri = "/logo.svg";
	logoUri.method = HTTP_GET;
	logoUri.handler = this->logoGetHandler;

	httpd_uri_t manifestUri;
	manifestUri.uri = "/manifest.json";
	manifestUri.method = HTTP_GET;
	manifestUri.handler = this->manifestGetHandler;

	httpd_uri_t postUri;
	postUri.uri = "/api";
	postUri.method = HTTP_POST;
	postUri.handler = this->apiPostHandler;

	httpd_uri_t optionsUri;
	optionsUri.uri = "/api";
	optionsUri.method = HTTP_OPTIONS;
	optionsUri.handler = this->apiOptionsHandler;

	httpd_uri_t otherUri;
	otherUri.uri = "/*";
	otherUri.method = HTTP_GET;
	otherUri.handler = this->otherGetHandler;

	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	// whiout this the esp crashed whitout a proper warning
	config.stack_size = 20480;
	config.uri_match_fn = httpd_uri_match_wildcard;

	// Start the httpd server
	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) == ESP_OK)
	{
		// Set URI handlers
		httpd_register_uri_handler(server, &indexUri);
		httpd_register_uri_handler(server, &logoUri);
		httpd_register_uri_handler(server, &manifestUri);
		httpd_register_uri_handler(server, &otherUri);
		httpd_register_uri_handler(server, &postUri);
		httpd_register_uri_handler(server, &optionsUri);
		return server;
	}

	ESP_LOGI(TAG, "Error starting server!");
	return NULL;
}

void BottleFiller::stopWebserver(httpd_handle_t server)
{
	// Stop the httpd server
	httpd_stop(server);
}

esp_err_t BottleFiller::indexGetHandler(httpd_req_t *req)
{
	// ESP_LOGI(TAG, "index_get_handler");
	extern const unsigned char index_html_start[] asm("_binary_index_html_gz_start");
	extern const unsigned char index_html_end[] asm("_binary_index_html_gz_end");
	const size_t index_html_size = (index_html_end - index_html_start);
	httpd_resp_set_type(req, "text/html");
	httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
	httpd_resp_send(req, (const char *)index_html_start, index_html_size);

	return ESP_OK;
}

esp_err_t BottleFiller::logoGetHandler(httpd_req_t *req)
{
	extern const unsigned char logo_svg_file_start[] asm("_binary_logo_svg_gz_start");
	extern const unsigned char logo_svg_file_end[] asm("_binary_logo_svg_gz_end");
	const size_t logo_svg_file_size = (logo_svg_file_end - logo_svg_file_start);
	httpd_resp_set_type(req, "image/svg+xml");
	httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
	httpd_resp_send(req, (const char *)logo_svg_file_start, logo_svg_file_size);

	return ESP_OK;
}

esp_err_t BottleFiller::manifestGetHandler(httpd_req_t *req)
{
	extern const unsigned char manifest_json_file_start[] asm("_binary_manifest_json_start");
	extern const unsigned char manifest_json_file_end[] asm("_binary_manifest_json_end");
	const size_t manifest_json_file_size = (manifest_json_file_end - manifest_json_file_start);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, (const char *)manifest_json_file_start, manifest_json_file_size);

	return ESP_OK;
}

esp_err_t BottleFiller::otherGetHandler(httpd_req_t *req)
{
	httpd_resp_set_status(req, "307 Temporary Redirect");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, "<html><body>Wrong</body></html>", 0); // Response body can be empty

	return ESP_OK;
}

esp_err_t BottleFiller::apiPostHandler(httpd_req_t *req)
{
	string stringBuffer;
	char buf[256];
	uint32_t ret;
	uint32_t remaining = req->content_len;

	while (remaining > 0)
	{
		// Read the data
		int nBytes = (std::min<uint32_t>)(remaining, sizeof(buf));

		if ((ret = httpd_req_recv(req, buf, nBytes)) <= 0)
		{
			if (ret == HTTPD_SOCK_ERR_TIMEOUT)
			{
				// Timeout, just continue
				continue;
			}

			return ESP_FAIL;
		}

		size_t bytes_read = ret;

		remaining -= bytes_read;

		// append to buffer
		stringBuffer.append((char *)buf, bytes_read);
	}

	string commandResult = mainInstance->processCommand(stringBuffer);

	const char *returnBuf = commandResult.c_str();
	httpd_resp_set_type(req, "text/plain");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	httpd_resp_sendstr(req, returnBuf);

	return ESP_OK;
}

// needed for cors
esp_err_t BottleFiller::apiOptionsHandler(httpd_req_t *req)
{
	string commandResult = "";
	const char *returnBuf = commandResult.c_str();

	httpd_resp_set_type(req, "text/plain");
	httpd_resp_set_hdr(req, "Access-Control-Max-Age", "1728000");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Authorization,Content-Type,Accept,Origin,User-Agent,DNT,Cache-Control,X-Mx-ReqToken,Keep-Alive,X-Requested-With,If-Modified-Since");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	httpd_resp_set_status(req, "204");
	httpd_resp_sendstr(req, returnBuf);

	return ESP_OK;
}

string BottleFiller::to_iso_8601(std::chrono::time_point<std::chrono::system_clock> t)
{

	// convert to time_t which will represent the number of
	// seconds since the UNIX epoch, UTC 00:00:00 Thursday, 1st. January 1970
	auto epoch_seconds = std::chrono::system_clock::to_time_t(t);

	// Format this as date time to seconds resolution
	// e.g. 2016-08-30T08:18:51
	std::stringstream stream;
	stream << std::put_time(gmtime(&epoch_seconds), "%FT%T");

	// If we now convert back to a time_point we will get the time truncated
	// to whole seconds
	auto truncated = std::chrono::system_clock::from_time_t(epoch_seconds);

	// Now we subtract this seconds count from the original time to
	// get the number of extra microseconds..
	auto delta_us = std::chrono::duration_cast<std::chrono::microseconds>(t - truncated).count();

	// And append this to the output stream as fractional seconds
	// e.g. 2016-08-30T08:18:51.867479
	stream << "." << std::fixed << std::setw(6) << std::setfill('0') << delta_us;

	return stream.str();
}