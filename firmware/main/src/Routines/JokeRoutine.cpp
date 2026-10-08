#include "JokeRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <cstdlib>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"

DEFINE_LOG(JokeRoutine)

Routine::TickingState JokeRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();

    if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || com == nullptr){
        CMF_LOG(JokeRoutine, LogLevel::Error, "Missing required service(s)");
        abort();
    }

    const int16_t id = Phrases::get(Phrase::Joke);
    if(id < 0){
        CMF_LOG(JokeRoutine, LogLevel::Warning, "Phrases::get returned no phrase id");
    	return TickingState::Done;
    }

    JokeData jokeData{};
    jokeData.id = id;
    com->sendData(BB::State::Scenario, BB::Action::Scenario::Joke, jokeData);

    auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::Joke, id));
    audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
    audio->waitEnd(portMAX_DELAY);

	return TickingState::Done;
}
