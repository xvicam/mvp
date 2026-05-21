import { useEffect, useState } from 'react'
import heroImg from './assets/hero.png'
import dashboardIllustration from './assets/dashboard-illustration.svg'
import deviceIllustration from './assets/device-illustration.svg'
import './App.css'

function App() {
  const [isDark, setIsDark] = useState(false)

  useEffect(() => {
    const root = document.documentElement
    if (isDark) {
      root.classList.add('dark')
    } else {
      root.classList.remove('dark')
    }
  }, [isDark])

  const team = [
    { name: 'Hardware + IoT', role: 'Team Member 1', linkedin: '#', github: '#' },
    { name: 'Embedded + Safety', role: 'Team Member 2', linkedin: '#', github: '#' },
    { name: 'UX + Frontend', role: 'Team Member 3', linkedin: '#', github: '#' },
    { name: 'Research + Validation', role: 'Team Member 4', linkedin: '#', github: '#' },
    { name: 'Product + Testing', role: 'Team Member 5', linkedin: '#', github: '#' },
  ]

  const highlights = [
    { label: '60m Range', detail: 'Early bike detection' },
    { label: 'Sees Corners', detail: 'Works through walls/vans' },
    { label: 'Tactile Alerts', detail: 'Steering wheel vibrates' },
    { label: 'Auto SOS', detail: 'Crash help system' },
  ]

  const features = [
    {
      title: 'Smart Detection',
      description: 'Sensors spot bikes automatically, even behind buildings.',
    },
    {
      title: 'Gentle Vibrations',
      description: 'The wheel vibrates softly as bikes approach—scaling with distance.',
    },
    {
      title: 'No False Alarms',
      description: 'Ignores parked bikes to prevent annoying, constant alerts.',
    },
    {
      title: 'Crash SOS',
      description: 'Calls for emergency help if the cyclist has a hard fall.',
    },
  ]

  return (
    <div className="min-h-screen bg-slate-50 text-slate-900 transition-colors duration-300 dark:bg-slate-950 dark:text-slate-100">
      <header className="sticky top-0 z-40 border-b border-slate-200/70 bg-white/80 backdrop-blur dark:border-slate-800/70 dark:bg-slate-950/80">
        <div className="mx-auto flex w-full max-w-6xl items-center justify-between px-6 py-4">
          <div className="flex items-center gap-3">
            <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-blue-600 text-white shadow-lg shadow-blue-500/30">
              <span className="text-lg font-semibold">V</span>
            </div>
            <div>
              <p className="text-sm font-semibold uppercase tracking-[0.2em] text-blue-700 dark:text-blue-400">
                VICAM
              </p>
              <p className="text-xs text-slate-500 dark:text-slate-400">Vehicle-Integrated Cyclist Alert Module</p>
            </div>
          </div>
          <nav className="hidden items-center gap-6 text-sm font-medium text-slate-600 dark:text-slate-300 md:flex">
            <a className="hover:text-blue-600 transition" href="#problem">Problem</a>
            <a className="hover:text-blue-600 transition" href="#solution">Solution</a>
            <a className="hover:text-blue-600 transition" href="#demo">Demo</a>
            <a className="hover:text-blue-600 transition" href="#poster">Poster</a>
          </nav>
          <button
            type="button"
            onClick={() => setIsDark((value) => !value)}
            className="inline-flex items-center gap-2 rounded-full border border-slate-200 bg-white px-4 py-2 text-sm font-medium text-slate-700 shadow-sm transition hover:border-blue-400 hover:text-blue-600 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-200"
            aria-label="Toggle dark mode"
          >
            <span className="flex h-6 w-6 items-center justify-center rounded-full bg-blue-100 text-blue-700 dark:bg-blue-500/20 dark:text-blue-300">
              {isDark ? (
                <svg viewBox="0 0 24 24" className="h-4 w-4" role="img" aria-label="Dark">
                  <path
                    fill="currentColor"
                    d="M12.8 3.2a1 1 0 0 1 1.1 1.5A7 7 0 1 0 19.3 14a1 1 0 0 1 1.5 1.1A9 9 0 1 1 12.8 3.2z"
                  />
                </svg>
              ) : (
                <svg viewBox="0 0 24 24" className="h-4 w-4" role="img" aria-label="Light">
                  <path
                    fill="currentColor"
                    d="M12 5a1 1 0 0 1 1 1v1a1 1 0 1 1-2 0V6a1 1 0 0 1 1-1zm0 10a3 3 0 1 0 0-6 3 3 0 0 0 0 6zm7-4a1 1 0 0 1 1 1v1a1 1 0 1 1-2 0v-1a1 1 0 0 1 1-1zM6 12a1 1 0 0 1-1 1H4a1 1 0 1 1 0-2h1a1 1 0 0 1 1 1zm13.07-5.07a1 1 0 0 1 0 1.41l-.7.7a1 1 0 1 1-1.42-1.41l.71-.7a1 1 0 0 1 1.41 0zM7.05 16.95a1 1 0 0 1 0 1.41l-.7.7a1 1 0 1 1-1.42-1.41l.71-.7a1 1 0 0 1 1.41 0zM19.07 16.95a1 1 0 0 1 1.41 0l.7.7a1 1 0 1 1-1.41 1.41l-.7-.7a1 1 0 0 1 0-1.41zM7.05 7.05a1 1 0 0 1-1.41 0l-.7-.7a1 1 0 0 1 1.41-1.41l.7.7a1 1 0 0 1 0 1.41z"
                  />
                </svg>
              )}
            </span>
            {isDark ? 'Dark' : 'Light'}
          </button>
        </div>
      </header>

      <main className="mx-auto w-full max-w-5xl px-6 pb-24 pt-12">
        
        {/* HERO */}
        <section className="grid items-center gap-10 lg:grid-cols-[1.1fr_0.9fr]">
          <div className="space-y-5">
            <span className="inline-flex w-fit items-center gap-2 rounded-full bg-blue-100 px-4 py-1 text-xs font-semibold uppercase tracking-[0.3em] text-blue-700 dark:bg-blue-500/15 dark:text-blue-300">
              Innovation Fest
            </span>
            <h1 className="text-4xl font-semibold leading-tight text-slate-900 dark:text-white sm:text-5xl">
              Spot Cyclists. Before You See Them.
            </h1>
            <p className="text-lg text-slate-600 dark:text-slate-300 leading-relaxed">
              VICAM protects cyclists at blind corners. Smart sensors talk to each other, giving drivers a soft steering wheel vibration when a bike is near. Safe, silent, and early.
            </p>
            <div className="flex flex-wrap items-center gap-4 pt-2">
              <a
                className="rounded-full bg-blue-600 px-6 py-3 text-sm font-semibold text-white shadow-lg shadow-blue-500/30 transition hover:bg-blue-500"
                href="#demo"
              >
                Watch live demo
              </a>
              <a
                className="rounded-full border border-slate-200 bg-white px-6 py-3 text-sm font-semibold text-slate-700 transition hover:border-blue-400 hover:text-blue-600 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-200"
                href="/VICAM_Poster_A1.pdf"
                target="_blank"
                rel="noreferrer"
              >
                View our poster
              </a>
            </div>
          </div>

          <div className="relative">
            <div className="absolute -top-10 right-0 hidden h-28 w-28 rounded-full bg-blue-400/30 blur-3xl lg:block"></div>
            <div className="grid gap-3 sm:grid-cols-2">
              {highlights.map((item) => (
                <div
                  key={item.label}
                  className="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm transition hover:border-blue-300 dark:border-slate-800 dark:bg-slate-900"
                >
                  <p className="text-xl font-bold text-blue-600 dark:text-blue-400">{item.label}</p>
                  <p className="text-sm font-medium text-slate-600 dark:text-slate-400 mt-1">{item.detail}</p>
                </div>
              ))}
            </div>
          </div>
        </section>

        {/* PROBLEM & SOLUTION */}
        <section className="mt-24 grid gap-8 lg:grid-cols-2">
          {/* Problem */}
          <div className="rounded-3xl border border-slate-200 bg-white p-8 dark:border-slate-800 dark:bg-slate-900">
            <p className="text-sm font-semibold uppercase tracking-[0.3em] text-blue-600">The Problem</p>
            <h2 className="text-3xl font-semibold mt-2">Blind spots cause accidents.</h2>
            <p className="mt-4 text-slate-600 dark:text-slate-300">
              Cameras fail at covered junctions. Loud alarms panic drivers. We need a silent warning before the bike even appears.
            </p>
            <div className="mt-8 grid gap-4">
              <div className="rounded-2xl bg-slate-50 p-4 dark:bg-slate-800/60">
                <p className="font-semibold text-slate-900 dark:text-white">Can't See Through Walls</p>
                <p className="text-sm text-slate-600 dark:text-slate-400 mt-1">Normal cameras are blind on corners.</p>
              </div>
              <div className="rounded-2xl bg-slate-50 p-4 dark:bg-slate-800/60">
                <p className="font-semibold text-slate-900 dark:text-white">Late Reactions</p>
                <p className="text-sm text-slate-600 dark:text-slate-400 mt-1">A split-second delay leads to near-misses.</p>
              </div>
            </div>
          </div>

          {/* Solution */}
          <div className="rounded-3xl border border-blue-200 bg-blue-50/50 p-8 dark:border-blue-900/30 dark:bg-blue-900/10">
            <p className="text-sm font-semibold uppercase tracking-[0.3em] text-blue-600">The Solution</p>
            <h2 className="text-3xl font-semibold mt-2">Silent, early awareness.</h2>
            <p className="mt-4 text-slate-600 dark:text-slate-300">
              The bike broadcasts a hidden signal. The car receives it. The steering wheel vibrates. Perfect safety without distraction.
            </p>
            <div className="mt-8 grid gap-4 sm:grid-cols-2">
              {features.map((feature) => (
                <div key={feature.title} className="rounded-2xl bg-white p-4 shadow-sm dark:bg-slate-900 dark:border dark:border-slate-800">
                  <p className="font-semibold text-slate-900 dark:text-white text-sm">{feature.title}</p>
                  <p className="text-xs text-slate-600 dark:text-slate-400 mt-2 leading-relaxed">{feature.description}</p>
                </div>
              ))}
            </div>
          </div>
        </section>

        {/* DEMO / TOP-DOWN VISUAL */}
        <section id="demo" className="mt-24">
          <div className="text-center max-w-2xl mx-auto space-y-4 mb-10">
            <p className="text-sm font-semibold uppercase tracking-[0.3em] text-blue-600">Live Simulation</p>
            <h2 className="text-3xl font-semibold">How it works on the road.</h2>
            <p className="text-slate-600 dark:text-slate-300">
              Watch real-time signal flow. The car receives early warning through a solid building block, allowing the driver to brake early.
            </p>
          </div>

          <div className="relative w-full h-[460px] bg-slate-800 rounded-3xl overflow-hidden shadow-2xl border border-slate-700">
            {/* Road Background Dashed Line */}
            <div className="absolute left-0 right-0 top-1/2 border-t-8 border-dashed border-slate-600 transform -translate-y-1/2"></div>
            
            {/* Obstacle / Building on the bottom edge representing blind corner */}
            <div className="absolute bottom-0 left-1/2 transform -translate-x-1/2 w-48 h-[60%] bg-slate-900 border-t-4 border-r-4 border-l-4 border-slate-700 flex items-start justify-center pt-6 rounded-t-xl z-20 shadow-2xl">
              <span className="text-slate-500 text-xs font-bold uppercase tracking-[0.2em] bg-slate-800 px-3 py-1 rounded-full">Blind Corner</span>
            </div>

            {/* Signal Flow (Travels Right to Left, through the obstacle) */}
            <div className="absolute top-[30%] left-[20%] right-[20%] text-blue-400 z-30">
               <div className="relative w-full border-t border-dashed border-blue-400/40">
                  <div className="absolute top-[-16px] animate-signal-flow">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={2.5} stroke="currentColor" className="w-8 h-8">
                      <path strokeLinecap="round" strokeLinejoin="round" d="M15.75 19.5 8.25 12l7.5-7.5" />
                    </svg>
                  </div>
               </div>
            </div>

            <div className="absolute inset-0 flex items-center justify-between px-10 sm:px-20 z-10 pointer-events-none">
              
              {/* Driver Area (Left) */}
              <div className="flex flex-col items-center transform -translate-y-10">
                <div className="w-24 h-40 bg-zinc-300 rounded-3xl shadow-xl flex flex-col justify-start items-center border-[3px] border-zinc-400 relative">
                  {/* Windshield */}
                  <div className="w-20 h-10 bg-slate-900 mt-6 rounded-md border-b-2 border-zinc-500 relative flex justify-center items-end pb-1">
                    {/* Glowing Vibrating Steering Wheel inside */}
                    <div className="animate-vibrate text-blue-400 w-5 h-5 filter drop-shadow-[0_0_8px_rgba(96,165,250,0.8)]">
                      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                        <circle cx="12" cy="12" r="10" />
                        <path d="M12 12 l-7.5 5.5 M12 12 l7.5 5.5 M12 12 v8" />
                        <circle cx="12" cy="12" r="3" fill="currentColor" />
                      </svg>
                    </div>
                  </div>
                  {/* Taillights */}
                  <div className="absolute bottom-1 left-2 w-3 h-2 bg-red-500 rounded-sm"></div>
                  <div className="absolute bottom-1 right-2 w-3 h-2 bg-red-500 rounded-sm"></div>
                </div>
                {/* Context Label */}
                <div className="mt-4 bg-slate-900/90 backdrop-blur px-4 py-2 border border-slate-700 rounded-xl text-center shadow-lg">
                  <p className="text-sm font-bold text-white">Car Receives Signal</p>
                  <p className="text-xs text-blue-400 font-medium">Wheel vibrates to warn driver</p>
                </div>
              </div>

              {/* Cyclist Area (Right) */}
              <div className="flex flex-col items-center transform translate-y-12">
                <div className="relative w-16 h-24 flex items-center justify-center">
                  <div className="absolute w-28 h-28 bg-blue-500/20 rounded-full animate-ping"></div>
                  <div className="w-6 h-16 bg-blue-500 rounded-full shadow-lg border border-blue-400 flex items-center justify-center text-white z-10 relative">
                    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" className="w-10 h-10 absolute -left-2 top-2">
                      <path d="M15.5 5.5c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zM5 12c-2.8 0-5 2.2-5 5s2.2 5 5 5 5-2.2 5-5-2.2-5-5-5zm0 8.5c-1.9 0-3.5-1.6-3.5-3.5s1.6-3.5 3.5-3.5 3.5 1.6 3.5 3.5-1.6 3.5-3.5 3.5zm5.8-10l2.4-2.4 .8 .8c1.3 1.3 3 2.1 5.1 2.1V9c-1.5 0-2.7-.6-3.6-1.5l-1.9-1.9c-.5-.4-1-.7-1.6-.7s-1.1.2-1.4.6L8.5 8.4c-.3.4-.5.9-.5 1.4v5h2v-4l1.8-1.5zM19 12c-2.8 0-5 2.2-5 5s2.2 5 5 5 5-2.2 5-5-2.2-5-5-5zm0 8.5c-1.9 0-3.5-1.6-3.5-3.5s1.6-3.5 3.5-3.5 3.5 1.6 3.5 3.5-1.6 3.5-3.5 3.5z" />
                    </svg>
                  </div>
                </div>
                {/* Context Label */}
                <div className="mt-4 bg-slate-900/90 backdrop-blur px-4 py-2 border border-slate-700 rounded-xl text-center shadow-lg z-20">
                  <p className="text-sm font-bold text-white">Hidden Cyclist</p>
                  <p className="text-xs text-blue-400 font-medium">Broadcasts location early</p>
                </div>
              </div>

            </div>
          </div>
        </section>

        {/* POSTER CTA */}
        <section id="poster" className="mt-24">
          <div className="rounded-3xl border border-slate-200 bg-white p-10 text-center shadow-sm dark:border-slate-800 dark:bg-slate-900">
            <h2 className="text-3xl font-semibold">Dig into the details.</h2>
            <p className="mt-3 text-slate-600 dark:text-slate-300 max-w-xl mx-auto">
              Scan our poster for full research diagrams, hardware specs, and testing results.
            </p>
            <div className="mt-6 flex flex-wrap justify-center gap-4">
              <a
                className="rounded-full bg-blue-600 px-8 py-3 font-semibold text-white shadow-lg shadow-blue-500/30 transition hover:bg-blue-500"
                href="/VICAM_Poster_A1.pdf"
                target="_blank"
                rel="noreferrer"
              >
                Open Poster PDF
              </a>
            </div>
          </div>
        </section>

        {/* TEAM & FEEDBACK (Side by Side) */}
        <section id="feedback" className="mt-24 grid gap-10 lg:grid-cols-2 items-start">
          
          <div className="space-y-6">
            <div>
              <p className="text-sm font-semibold uppercase tracking-[0.3em] text-blue-600">The Team</p>
              <h2 className="text-2xl font-semibold mt-1">Built for safer streets.</h2>
            </div>
            
            <div className="grid gap-3">
              {team.map((member) => (
                <div key={member.name} className="flex items-center justify-between p-4 rounded-2xl border border-slate-200 bg-white dark:border-slate-800 dark:bg-slate-900">
                  <div>
                    <p className="font-semibold text-sm text-slate-900 dark:text-white">{member.name}</p>
                    <p className="text-xs text-slate-500 dark:text-slate-400">{member.role}</p>
                  </div>
                  <div className="flex gap-2">
                    <a className="text-xs font-semibold text-blue-600 hover:underline dark:text-blue-400" href={member.linkedin}>LinkedIn</a>
                    <a className="text-xs font-semibold text-blue-600 hover:underline dark:text-blue-400" href={member.github}>GitHub</a>
                  </div>
                </div>
              ))}
            </div>
          </div>

          <div className="rounded-3xl border border-slate-200 bg-white p-8 shadow-sm dark:border-slate-800 dark:bg-slate-900">
            <h2 className="text-2xl font-semibold mb-6">Leave us feedback</h2>
            <form className="grid gap-4" onSubmit={(event) => event.preventDefault()}>
              <input
                className="w-full rounded-2xl border border-slate-200 bg-slate-50 px-4 py-3 text-sm text-slate-900 focus:border-blue-400 focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-white focus:ring-1 focus:ring-blue-400"
                type="text"
                placeholder="Name or Role (Optional)"
              />
              <textarea
                className="w-full min-h-[120px] rounded-2xl border border-slate-200 bg-slate-50 px-4 py-3 text-sm text-slate-900 focus:border-blue-400 focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-white focus:ring-1 focus:ring-blue-400"
                placeholder="What did you think of our project?"
              ></textarea>
              <button
                type="submit"
                className="rounded-full bg-blue-600 px-6 py-3 text-sm font-semibold text-white shadow-lg shadow-blue-500/30 transition hover:bg-blue-500"
              >
                Submit
              </button>
            </form>
          </div>
        </section>
      </main>

      <footer className="border-t border-slate-200/70 bg-white/80 py-8 text-center text-sm font-medium text-slate-500 dark:border-slate-800/70 dark:bg-slate-950/80 dark:text-slate-400">
        VICAM Innovation Fest Showcase 2026
      </footer>
    </div>
  )
}

export default App
