// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <cstdint>
#include <string>  // for char_traits, operator+, string, basic_string

#include "ftxui/component/component.hpp"       // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/elements.hpp"  // for text, hbox, separator, Element, operator|, vbox, border
#include "ftxui/util/ref.hpp"  // for Ref

#include <ftxui/dom/elements.hpp>
#include <random>
#include <thread>
#include "generated_wordlist.h"

int main() {
  // https://raw.githubusercontent.com/powerlanguage/word-lists/refs/heads/master/1000-most-common-words.txt

  std::default_random_engine generator;
  std::uniform_int_distribution<uint64_t> distribution(
      0, random_string_list.size() - 1);
  auto dice = std::bind(distribution, generator);
  std::deque<char> string_buffer_dequeue;
  uint64_t character_count{0}, mistake_count{0}, word_count{0};
  std::atomic<bool> start_program_flag = false;
  std::atomic<bool> halt_program = false;

  auto generate_string_buffer = [&] {
    string_buffer_dequeue.clear();
    constexpr auto max_len = 200;

    for (int i = 0; i < max_len; ++i) {
      for (auto& j : random_string_list[dice()]) {
        string_buffer_dequeue.push_back(j);
      }

      string_buffer_dequeue.push_back(' ');
    }
  };

  generate_string_buffer();

  std::atomic<uint64_t> timer_counter{30};
  constexpr auto total_time{30};

  using namespace ftxui;

  // The data:
  std::string input_text;

  // The basic input components:
  Component input_text_component = Input(&input_text, "");

  input_text_component |= CatchEvent([&](Event event) {
    bool char_matched = string_buffer_dequeue.front() == event.character()[0];

    if (event.is_character()) {
      start_program_flag.store(true);

      if (!halt_program) {
        if (char_matched) {
          if (string_buffer_dequeue.front() == ' ') {
            ++word_count;
          }

          string_buffer_dequeue.pop_front();
          if (string_buffer_dequeue.empty()) {
            generate_string_buffer();
          }

          ++character_count;
        } else {
          ++mistake_count;
        }
      }
    }

    return event.is_character() && !char_matched;
  });

  // The component tree:
  auto component = Container::Vertical({input_text_component});

  // Tweak how the component tree is rendered:
  auto renderer = Renderer(component, [&] {
    std::string result(
        string_buffer_dequeue.begin(),
        string_buffer_dequeue.end());  // copy : ugly but paragrah
    double accuracy = 1 - (double)mistake_count / character_count;
    double wpm = accuracy * (character_count / 5.0) * (60.0 / total_time);
    if (halt_program) {
        return paragraph("PROGRAM END") | bold;
    } else
{    return vbox({
               hbox({
                   vbox({
                       text("INPUT STRING: \n") | size(HEIGHT, EQUAL, 1) | bold,
                       paragraph(result) | ftxui::underlined |
                           size(WIDTH, EQUAL, 120),  // TODO: Fix screen size
                   }),

                   separator(),
                   vbox({
                       paragraph(std::format("Timer Value: {}",
                                             timer_counter.load())),
                       paragraph(
                           std::format("Character Count: {}", character_count)),
                       paragraph(
                           std::format("Mistake Count: {}", mistake_count)),
                       paragraph(std::format("Word Count: {}", word_count)),
                       paragraph(
                           std::format("Accuracy: {:.2f}", 100 * accuracy)),
                       paragraph(std::format("WPM: {:.2f}", wpm)),

                   }),
               }),
               separator(),
               hbox(text(" Input Text Here : "),
                    input_text_component->Render()),
           }) |
           border;
  }});

  auto screen = ScreenInteractive::Fullscreen();
  auto loop_thread = std::jthread([&] { screen.Loop(renderer); });

  auto timer_thread = std::jthread([&] {
    using namespace std::literals;
    while (1) {
      if (timer_counter <= 0) {
        halt_program = true;
      }
      if (timer_counter > 0 && start_program_flag.load()) {
        std::this_thread::sleep_for(1s);
        --timer_counter;
        screen.PostEvent(ftxui::Event::Custom);
      }
    }
  });
}