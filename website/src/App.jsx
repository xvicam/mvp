import { useEffect, useState } from 'react'
import heroImg from './assets/hero.png'
import dashboardIllustration from './assets/dashboard-illustration.svg'
import deviceIllustration from './assets/device-illustration.svg'
import './App.css'

// Reusable Top-Down Blind Corner Junction Simulation Component
function JunctionSimulation({ isDark }) {
  return (
    <div className="relative w-full bg-slate-950 rounded-3xl overflow-hidden shadow-2xl border border-slate-800 flex flex-col items-center justify-center select-none p-4 md:p-6">
      
      {/* Simulation Heading / Visual Subtitle */}
      <div className="w-full flex items-center justify-between mb-4 border-b border-slate-800/60 pb-3">
        <div className="flex flex-wrap items-center gap-3">
          <div className="flex items-center gap-2">
            <span className="flex h-2.5 w-2.5 rounded-full bg-green-500 animate-ping"></span>
            <span className="text-xs font-bold uppercase tracking-[0.15em] text-green-400">Live Active Simulation</span>
          </div>
          <span className="hidden md:inline text-slate-700">|</span>
          <div className="flex items-center gap-2 bg-red-950/70 border border-red-800/40 px-3 py-1 rounded-xl shadow-[0_0_15px_rgba(239,68,68,0.15)] animate-pulse">
            <span className="flex h-2 w-2 rounded-full bg-red-500 animate-ping"></span>
            <span className="text-[10px] font-black uppercase text-red-400 tracking-wider">
              ⚠️ CRITICAL PROXIMITY: Cyclist Detected (44m)
            </span>
          </div>
        </div>
        <div className="text-[10px] text-slate-500 font-bold uppercase tracking-wider">
          Scale: 1:1 Safety Scenario
        </div>
      </div>

      {/* Main Responsive SVG Container */}
      <div className="w-full aspect-[8/5] relative">
        <svg 
          viewBox="0 0 800 500" 
          className="w-full h-full rounded-2xl overflow-hidden bg-slate-900/40 border border-slate-800/80 shadow-[inset_0_4px_30px_rgba(0,0,0,0.8)]"
        >
          <defs>
            {/* Tech grid background pattern */}
            <pattern id="grid" width="20" height="20" patternUnits="userSpaceOnUse">
              <path d="M 20 0 L 0 0 0 20" fill="none" stroke="rgba(255, 255, 255, 0.02)" strokeWidth="1" />
            </pattern>
            
            {/* Concrete wall tech pattern */}
            <pattern id="wall-pattern" width="16" height="16" patternUnits="userSpaceOnUse">
              <rect width="16" height="16" fill="transparent" />
              <circle cx="8" cy="8" r="1.5" fill="rgba(255, 255, 255, 0.08)" />
            </pattern>

            {/* Glowing Effects */}
            <filter id="glow-blue" x="-20%" y="-20%" width="140%" height="140%">
              <feGaussianBlur stdDeviation="6" result="blur" />
              <feComposite in="SourceGraphic" in2="blur" operator="over" />
            </filter>
            
            <filter id="glow-red" x="-20%" y="-20%" width="140%" height="140%">
              <feGaussianBlur stdDeviation="8" result="blur" />
              <feComposite in="SourceGraphic" in2="blur" operator="over" />
            </filter>

            {/* Gradient Background for Obstacle Wall */}
            <linearGradient id="concrete-gradient" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stopColor="#1e293b" />
              <stop offset="100%" stopColor="#0f172a" />
            </linearGradient>
            
            <linearGradient id="road-gradient" x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stopColor="#0b0f19" />
              <stop offset="100%" stopColor="#111625" />
            </linearGradient>

            <linearGradient id="blueGlow" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stopColor="#3b82f6" stopOpacity="0.8" />
              <stop offset="100%" stopColor="#60a5fa" stopOpacity="0.2" />
            </linearGradient>
          </defs>

          {/* 1. Grid Background Layer */}
          <rect width="800" height="500" fill="url(#grid)" />

          {/* 1.5. VICAM Active Detection Perimeter (60m safety bubble) */}
          <g>
            {/* Dotted circular zone centered at the vehicle (120, 360) reaching the cyclist at (560, 120) [r ≈ 501] */}
            <circle cx="120" cy="360" r="501" fill="rgba(59, 130, 246, 0.015)" stroke="#3b82f6" strokeWidth="2" strokeDasharray="8,6" opacity="0.3" filter="url(#glow-blue)">
              <animate attributeName="stroke-dashoffset" values="0;100" dur="20s" repeatCount="indefinite" />
            </circle>
            {/* Proximity range label along the detection circle path (top-right area) */}
            <g transform="translate(480, 100) rotate(-28.6)">
              <text fill="#60a5fa" fontSize="9" fontWeight="900" letterSpacing="0.1em" opacity="0.7">
                📡 VICAM DETECTION LIMIT (60m)
              </text>
            </g>
          </g>

          {/* 2. Road Layout Elements */}
          {/* Main Asphalt Roads - Horizontal Top road and Vertical Left road */}
          {/* Horizontal Road (height: 100, centered at y: 120) */}
          <rect x="0" y="70" width="800" height="100" fill="url(#road-gradient)" stroke="rgba(255, 255, 255, 0.03)" strokeWidth="1" />
          {/* Vertical Road (width: 100, centered at x: 120) */}
          <rect x="70" y="170" width="100" height="330" fill="url(#road-gradient)" stroke="rgba(255, 255, 255, 0.03)" strokeWidth="1" />
          
          {/* Single Dotted Lane divider for ultra-simple layout */}
          {/* Horizontal Road Single Dotted line */}
          <line x1="0" y1="120" x2="800" y2="120" stroke="#eab308" strokeWidth="2" strokeDasharray="6,6" strokeOpacity="0.5" />
          
          {/* Vertical Road Single Dotted line */}
          <line x1="120" y1="170" x2="120" y2="500" stroke="#eab308" strokeWidth="2" strokeDasharray="6,6" strokeOpacity="0.5" />
          
          {/* White stop line markings */}
          <line x1="70" y1="170" x2="170" y2="170" stroke="#ffffff" strokeWidth="3" strokeOpacity="0.4" />
          <line x1="170" y1="70" x2="170" y2="170" stroke="#ffffff" strokeWidth="3" strokeOpacity="0.4" />

          {/* Directional helper arrows */}
          {/* Upward road arrow */}
          <g transform="translate(120, 460) scale(0.8)" opacity="0.3">
            <line x1="0" y1="20" x2="0" y2="-20" stroke="#ffffff" strokeWidth="2.5" strokeLinecap="round" />
            <path d="M -6 -8 L 0 -18 L 6 -8" fill="none" stroke="#ffffff" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" />
          </g>
          {/* Leftward road arrow */}
          <g transform="translate(730, 120) scale(0.8)" opacity="0.3">
            <line x1="20" y1="0" x2="-20" y2="0" stroke="#ffffff" strokeWidth="2.5" strokeLinecap="round" />
            <path d="M -8 -6 L -18 0 L -8 6" fill="none" stroke="#ffffff" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" />
          </g>

          {/* 2.5. Collision Course Trajectories & Blind Spot Threat Zone */}
          <g>
            {/* Trajectory path for Car (upwards towards junction) */}
            <line x1="120" y1="360" x2="120" y2="155" stroke="#f97316" strokeWidth="2" strokeDasharray="5,5" opacity="0.5" />
            
            {/* Trajectory path for Cyclist (leftwards towards junction) */}
            <line x1="560" y1="120" x2="155" y2="120" stroke="#f97316" strokeWidth="2" strokeDasharray="5,5" opacity="0.5" />
            
            {/* Pulsing Intersection Threat Zone at (120, 120) */}
            <g transform="translate(120, 120)">
              <circle cx="0" cy="0" r="32" fill="none" stroke="#ef4444" strokeWidth="2" strokeDasharray="4,4" opacity="0.75">
                <animate attributeName="r" values="28;40;28" dur="2.5s" repeatCount="indefinite" />
                <animate attributeName="opacity" values="0.8;0.3;0.8" dur="2.5s" repeatCount="indefinite" />
              </circle>
              <circle cx="0" cy="0" r="22" fill="#7f1d1d" fillOpacity="0.4" stroke="#ef4444" strokeWidth="1.5" />
              <text y="3" fill="#fca5a5" fontSize="9" fontWeight="950" letterSpacing="0.05em" textAnchor="middle" className="animate-pulse">
                ⚠️ HAZARD
              </text>
            </g>
          </g>

          {/* 3. Solid Obstacle - Massive Corner Concrete Building */}
          {/* Corner occupies bottom-right quadrant: x >= 170, y >= 170 */}
          <g>
            {/* The main structure */}
            <rect 
              x="170" 
              y="170" 
              width="630" 
              height="330" 
              fill="url(#concrete-gradient)" 
              stroke="#334155" 
              strokeWidth="2.5" 
              rx="16" 
              filter="drop-shadow(0 15px 30px rgba(0,0,0,0.9))"
            />
            {/* Micro Dot Texture for building facade */}
            <rect x="174" y="174" width="622" height="322" fill="url(#wall-pattern)" rx="12" />
            
            {/* Internal architectural lines */}
            <line x1="170" y1="210" x2="800" y2="210" stroke="#1e293b" strokeWidth="2.5" opacity="0.5" />
            <line x1="250" y1="170" x2="250" y2="500" stroke="#1e293b" strokeWidth="2.5" opacity="0.5" />
            
            {/* Simple centered building label */}
            <g transform="translate(485, 335)">
              <rect x="-180" y="-20" width="360" height="40" rx="10" fill="#0f172a" stroke="#475569" strokeWidth="1.5" opacity="0.9" />
              <text y="5" fill="#94a3b8" fontSize="12" fontWeight="800" letterSpacing="0.15em" textAnchor="middle">
                🧱 CONCRETE BUILDING - (Blocks eyesight)
              </text>
            </g>
          </g>

          {/* 4. Active VICAM RF Signal Wave rings propagating from Cyclist */}
          {/* Cyclist center: (560, 120) */}
          <g>
            <circle cx="560" cy="120" r="15" fill="none" stroke="#60a5fa" strokeWidth="2.5" opacity="0.9">
              <animate attributeName="r" values="15;360" dur="3s" repeatCount="indefinite" />
              <animate attributeName="opacity" values="0.9;0" dur="3s" repeatCount="indefinite" />
            </circle>
            <circle cx="560" cy="120" r="15" fill="none" stroke="#60a5fa" strokeWidth="1.5" opacity="0.9">
              <animate attributeName="r" values="15;360" dur="3s" begin="1s" repeatCount="indefinite" />
              <animate attributeName="opacity" values="0.9;0" dur="3s" begin="1s" repeatCount="indefinite" />
            </circle>
            <circle cx="560" cy="120" r="15" fill="none" stroke="#3b82f6" strokeWidth="1" opacity="0.9">
              <animate attributeName="r" values="15;360" dur="3s" begin="2s" repeatCount="indefinite" />
              <animate attributeName="opacity" values="0.9;0" dur="3s" begin="2s" repeatCount="indefinite" />
            </circle>
          </g>

          {/* 5. Glowing Blue Transmitted Wireless Data Path (Active Warning signal) */}
          {/* Flows directly from (560, 120) to (120, 360) through the concrete wall */}
          <path
            d="M 560 120 L 120 360"
            stroke="#60a5fa"
            strokeWidth="4"
            strokeDasharray="12,10"
            className="animate-signal-flow-path"
            filter="url(#glow-blue)"
          />

          {/* Wireless signal info label on top of path */}
          <g transform="translate(340, 240) rotate(-28.6)">
            {/* Glass pill for RF label */}
            <rect x="-120" y="-12" width="240" height="24" rx="8" fill="#172554" stroke="#3b82f6" strokeWidth="1.5" opacity="0.95" />
            <text y="3" fill="#93c5fd" fontSize="9" fontWeight="800" textAnchor="middle" letterSpacing="0.06em">
              📡 RF Signal (Penetrates Wall)
            </text>
          </g>

          {/* 6. Red Direct Line of Sight (Blocked - Split into two parts to show blockage) */}
          {/* Part 1: Car to Building edge (170, 333) */}
          <line x1="120" y1="360" x2="170" y2="333" stroke="#ef4444" strokeWidth="2.5" strokeDasharray="5,5" opacity="0.8" />
          
          {/* Part 2: Cyclist to Building edge (468, 170) */}
          <line x1="560" y1="120" x2="468" y2="170" stroke="#ef4444" strokeWidth="2.5" strokeDasharray="5,5" opacity="0.8" />



          {/* 7. Cyclist Area (Top Road, traveling leftwards) */}
          {/* Centered at (560, 120) */}
          <g transform="translate(560, 120)">
            {/* Pulsing Transmitter Beacon outer ring */}
            <circle cx="0" cy="0" r="28" fill="#3b82f6" fillOpacity="0.15" className="animate-ping" />
            
            {/* Top-Down Bicycle & Rider representation */}
            {/* Rear Wheel (detailed top-down view) */}
            <rect x="14" y="-3" width="10" height="6" rx="2" fill="#1e293b" stroke="#475569" strokeWidth="1" />
            <line x1="19" y1="-3" x2="19" y2="3" stroke="#94a3b8" strokeWidth="1" />
            
            {/* Front Wheel (detailed top-down view) */}
            <rect x="-24" y="-3" width="10" height="6" rx="2" fill="#1e293b" stroke="#475569" strokeWidth="1" />
            <line x1="-19" y1="-3" x2="-19" y2="3" stroke="#94a3b8" strokeWidth="1" />
            
            {/* Main Bike Frame (sleek metallic blue) */}
            <line x1="-19" y1="0" x2="14" y2="0" stroke="#3b82f6" strokeWidth="3" strokeLinecap="round" filter="url(#glow-blue)" />
            {/* Frame stays */}
            <line x1="4" y1="0" x2="14" y2="-2" stroke="#1d4ed8" strokeWidth="1.5" />
            <line x1="4" y1="0" x2="14" y2="2" stroke="#1d4ed8" strokeWidth="1.5" />
            <line x1="-19" y1="-2" x2="-14" y2="0" stroke="#1d4ed8" strokeWidth="1.5" />
            <line x1="-19" y1="2" x2="-14" y2="0" stroke="#1d4ed8" strokeWidth="1.5" />
            
            {/* Handlebars (curved sleek bar) */}
            <path d="M-14 -12 C-14.5 -6 -14.5 6 -14 12" fill="none" stroke="#0f172a" strokeWidth="3" strokeLinecap="round" />
            {/* Handlebar Grips */}
            <rect x="-15.5" y="-12" width="2.5" height="3" rx="0.5" fill="#475569" />
            <rect x="-15.5" y="9" width="2.5" height="3" rx="0.5" fill="#475569" />
            
            {/* Streamlined Saddle/Seat */}
            <path d="M 4 -3 C 2.5 -3 1.5 -1.5 1.5 0 C 1.5 1.5 2.5 3 4 3 L 10 1.5 C 11.5 1 11.5 -1 10 -1.5 Z" fill="#0f172a" stroke="#1e293b" strokeWidth="1" />
            
            {/* Cyclist Torso / Shoulders (renders rider on the bike) */}
            <ellipse cx="-2" cy="0" rx="9" ry="12" fill="#1e293b" stroke="#334155" strokeWidth="1.5" />
            {/* Cycling jersey central stripe */}
            <rect x="-11" y="-2" width="18" height="4" fill="#3b82f6" opacity="0.8" />
            
            {/* Cyclist Arms reaching to handlebars */}
            <path d="M-2 -8 Q-10 -11 -14 -9" fill="none" stroke="#1e293b" strokeWidth="2" strokeLinecap="round" />
            <path d="M-2 8 Q-10 11 -14 9" fill="none" stroke="#1e293b" strokeWidth="2" strokeLinecap="round" />
            <circle cx="-14" cy="-9" r="2" fill="#e2e8f0" />
            <circle cx="-14" cy="9" r="2" fill="#e2e8f0" />

            {/* Cyclist Legs/Thighs pedaling */}
            <path d="M 2 -6 Q 6 -12 2 -12" fill="none" stroke="#1e293b" strokeWidth="2.5" strokeLinecap="round" />
            <path d="M 2 6 Q 6 12 6 12" fill="none" stroke="#1e293b" strokeWidth="2.5" strokeLinecap="round" />
            {/* Pedals */}
            <rect x="0" y="-14" width="4" height="2" fill="#475569" />
            <rect x="4" y="12" width="4" height="2" fill="#475569" />
            
            {/* Pulsing Cyclist Helmet Beacon (Active system) */}
            <path d="M -9 0 C -9 -5.5 -2 -5 -2 0 C -2 5 -9 5 -9 0" fill="#3b82f6" stroke="#60a5fa" strokeWidth="1" />
            <circle cx="-5" cy="0" r="6" fill="#60a5fa" className="animate-beacon" filter="url(#glow-blue)" />
            <circle cx="-5" cy="0" r="2.5" fill="#ffffff" />

            {/* Proximity / Riding direction Indicator Arrow */}
            <g transform="translate(-40, 0) rotate(180)" className="animate-pulse">
              <path d="M -6 -5 L 0 0 L -6 5" fill="none" stroke="#60a5fa" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" />
              <line x1="0" y1="0" x2="-15" y2="0" stroke="#60a5fa" strokeWidth="2.5" />
            </g>
          </g>

          {/* Cyclist warning tag (completely non-overlapping above the road) */}
          <g transform="translate(560, 65)">
            <rect x="-70" y="-12" width="140" height="24" rx="8" fill="#1e293b" stroke="#3b82f6" strokeWidth="1.5" opacity="0.95" />
            <text y="3" fill="#3b82f6" fontSize="10" fontWeight="900" letterSpacing="0.05em" textAnchor="middle">
              📡 Cyclist (Active)
            </text>
          </g>

          {/* 8. Car Area (Left Road, traveling upwards) */}
          {/* Centered at (120, 360) */}
          <g transform="translate(120, 360)">
            {/* Top-Down Detailed Vehicle SVG */}
            {/* Wheels */}
            <rect x="-24" y="-35" width="6" height="15" rx="2" fill="#334155" />
            <rect x="18" y="-35" width="6" height="15" rx="2" fill="#334155" />
            <rect x="-24" y="15" width="6" height="15" rx="2" fill="#334155" />
            <rect x="18" y="15" width="6" height="15" rx="2" fill="#334155" />
            
            {/* Car Chassis Body */}
            <rect x="-20" y="-45" width="40" height="85" rx="10" fill="#1e293b" stroke="#ef4444" strokeWidth="2" filter="url(#glow-red)" />
            <rect x="-17" y="-42" width="34" height="79" rx="8" fill="#0f172a" />
            
            {/* Windshield */}
            <rect x="-14" y="-22" width="28" height="18" rx="3" fill="#1e293b" stroke="#334155" strokeWidth="1" />
            
            {/* Warning Pulsing HUD inside the Windshield (Tactile Steer Wheel) */}
            <g transform="translate(0, -13)" className="animate-vibrate">
              <circle cx="0" cy="0" r="7.5" fill="none" stroke="#ef4444" strokeWidth="2" filter="url(#glow-red)" />
              <line x1="-5.5" y1="4.5" x2="5.5" y2="-4.5" stroke="#ef4444" strokeWidth="1.5" />
              <line x1="5.5" y1="4.5" x2="-5.5" y2="-4.5" stroke="#ef4444" strokeWidth="1.5" />
              <circle cx="0" cy="0" r="2.5" fill="#ef4444" />
            </g>

            {/* Glowing Headlights */}
            <path d="M-15 -45 L-25 -95 L-5 -95 Z" fill="url(#blueGlow)" opacity="0.1" />
            <path d="M15 -45 L5 -95 L25 -95 Z" fill="url(#blueGlow)" opacity="0.1" />
            <circle cx="-13" cy="-45" r="3.5" fill="#fef08a" filter="drop-shadow(0 0 5px #fef08a)" />
            <circle cx="13" cy="-45" r="3.5" fill="#fef08a" filter="drop-shadow(0 0 5px #fef08a)" />

            {/* Taillights */}
            <rect x="-16" y="38" width="6" height="2.5" fill="#dc2626" />
            <rect x="10" y="38" width="6" height="2.5" fill="#dc2626" />

            {/* Direction Indicator Arrow */}
            <g transform="translate(0, 60)" className="animate-bounce">
              <path d="M -5 -5 L 0 -10 L 5 -5" fill="none" stroke="#64748b" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" />
              <line x1="0" y1="-10" x2="0" y2="5" stroke="#64748b" strokeWidth="2.5" />
            </g>
          </g>

          {/* Car warning tag (completely non-overlapping above the car) */}
          <g transform="translate(120, 290)">
            <rect x="-105" y="-12" width="210" height="24" rx="8" fill="#7f1d1d" stroke="#ef4444" strokeWidth="1.5" opacity="0.95" filter="url(#glow-red)" />
            <text y="3" fill="#fca5a5" fontSize="9.5" fontWeight="900" letterSpacing="0.05em" textAnchor="middle">
              ⚠️ Cyclist Detected - Wheel Vibrates!
            </text>
          </g>
        </svg>
      </div>

      {/* 3-Step Walkthrough Legend specifically for Innovation Fest visitors */}
      <div className="w-full mt-6 grid gap-4 grid-cols-1 md:grid-cols-3 border-t border-slate-800/80 pt-5 text-left">
        
        {/* Step 1 */}
        <div className="bg-slate-900/40 border border-slate-800/60 p-3.5 rounded-2xl flex gap-3">
          <div className="h-8 w-8 shrink-0 rounded-xl bg-red-950/60 border border-red-900/60 flex items-center justify-center text-red-400 font-extrabold text-sm shadow-[0_0_10px_rgba(239,68,68,0.15)]">
            1
          </div>
          <div>
            <h4 className="text-xs font-black uppercase text-red-400 tracking-wider">Blind Corner Block</h4>
            <p className="text-[10px] text-slate-400 font-semibold mt-1 leading-relaxed">
              Concrete building creates absolute blind spot. Vision line is 100% blocked.
            </p>
          </div>
        </div>

        {/* Step 2 */}
        <div className="bg-slate-900/40 border border-slate-800/60 p-3.5 rounded-2xl flex gap-3">
          <div className="h-8 w-8 shrink-0 rounded-xl bg-blue-950/60 border border-blue-900/60 flex items-center justify-center text-blue-400 font-extrabold text-sm shadow-[0_0_10px_rgba(59,130,246,0.15)]">
            2
          </div>
          <div>
            <h4 className="text-xs font-black uppercase text-blue-400 tracking-wider">Wireless Beacon</h4>
            <p className="text-[10px] text-slate-400 font-semibold mt-1 leading-relaxed">
              Cyclist broadcasts 2.4GHz radio signal. Radio waves pass straight through concrete walls.
            </p>
          </div>
        </div>

        {/* Step 3 */}
        <div className="bg-slate-900/40 border border-slate-800/60 p-3.5 rounded-2xl flex gap-3">
          <div className="h-8 w-8 shrink-0 rounded-xl bg-emerald-950/60 border border-emerald-900/60 flex items-center justify-center text-emerald-400 font-extrabold text-sm shadow-[0_0_10px_rgba(16,185,129,0.15)]">
            3
          </div>
          <div>
            <h4 className="text-xs font-black uppercase text-emerald-400 tracking-wider">Early Tactile Warning</h4>
            <p className="text-[10px] text-slate-400 font-semibold mt-1 leading-relaxed">
              Car receives signal and vibrates wheel, alert-warning the driver long before visual contact.
            </p>
          </div>
        </div>

      </div>

    </div>
  )
}


function App() {
  const [isDark, setIsDark] = useState(false)
  const [isPresentationMode, setIsPresentationMode] = useState(false)
  const [activeSlide, setActiveSlide] = useState(0)
  const [isPlaying, setIsPlaying] = useState(true)
  const [slideDuration, setSlideDuration] = useState(8) // in seconds
  const [progress, setProgress] = useState(0)
  const [showControls, setShowControls] = useState(true)
  const [activeLightboxImg, setActiveLightboxImg] = useState(null)

  const slides = [
    { id: 'intro', label: 'Intro', title: 'Spot Cyclists. Before You See Them.' },
    { id: 'problem', label: 'Problem', title: 'Blind spots cause accidents.' },
    { id: 'solution', label: 'Solution', title: 'Silent, early awareness.' },
    { id: 'demo', label: 'Live Demo', title: 'How it works on the road.' },
    { id: 'poster', label: 'Poster', title: 'Dig into the details.' },
    { id: 'sequence', label: '10s Later', title: 'What happens 10 seconds later.' },
    { id: 'team', label: 'Team & Feedback', title: 'Built for safer streets.' },
  ]

  useEffect(() => {
    const root = document.documentElement
    if (isDark) {
      root.classList.add('dark')
    } else {
      root.classList.remove('dark')
    }
  }, [isDark])

  // Slide Auto-rotation Progress Loop
  useEffect(() => {
    if (!isPresentationMode || !isPlaying) return

    const intervalTime = 100 // update every 100ms
    const increment = 100 / (slideDuration * (1000 / intervalTime))

    const timer = setInterval(() => {
      setProgress((prev) => prev + increment)
    }, intervalTime)

    return () => clearInterval(timer)
  }, [isPresentationMode, isPlaying, slideDuration])

  // Watcher to advance slide when progress reaches 100
  useEffect(() => {
    if (progress >= 100) {
      setActiveSlide((current) => (current + 1) % slides.length)
      setProgress(0)
    }
  }, [progress, slides.length])

  // Reset showControls when presentation mode starts
  useEffect(() => {
    if (isPresentationMode) {
      setShowControls(true)
    }
  }, [isPresentationMode])

  const handleJumpToSlide = (index) => {
    setActiveSlide(index)
    setProgress(0)
  }

  const handleNextSlide = () => {
    setActiveSlide((current) => (current + 1) % slides.length)
    setProgress(0)
  }

  const handlePrevSlide = () => {
    setActiveSlide((current) => (current - 1 + slides.length) % slides.length)
    setProgress(0)
  }

  // Keyboard Navigation Hook
  useEffect(() => {
    if (!isPresentationMode) return

    const handleKeyDown = (e) => {
      switch (e.key) {
        case ' ':
          e.preventDefault()
          setIsPlaying((prev) => !prev)
          break
        case 'ArrowRight':
        case 'ArrowDown':
          e.preventDefault()
          handleNextSlide()
          break
        case 'ArrowLeft':
        case 'ArrowUp':
          e.preventDefault()
          handlePrevSlide()
          break
        case 'h':
        case 'H':
          e.preventDefault()
          setShowControls((prev) => !prev)
          break
        case 'Escape':
          e.preventDefault()
          setIsPresentationMode(false)
          break
        default:
          break
      }
    }

    window.addEventListener('keydown', handleKeyDown)
    return () => window.removeEventListener('keydown', handleKeyDown)
  }, [isPresentationMode, slides.length])

  // Body Scroll Lock Hook
  useEffect(() => {
    if (isPresentationMode) {
      document.body.classList.add('overflow-hidden')
    } else {
      document.body.classList.remove('overflow-hidden')
    }
    return () => document.body.classList.remove('overflow-hidden')
  }, [isPresentationMode])

  const team = [
    { name: "Baber Khan", url: "https://www.linkedin.com/in/baberr", course: "BSc Computer Science" },
    { name: "Ahmad Raza", url: "https://www.linkedin.com/in/ahmad-raza-45232a279/", course: "BSc Computer Science" },
    { name: "Saif Ahmed", url: "https://www.linkedin.com/in/saif-ahmed-aa8a18375", course: "BSc Computer Networks and Security" },
    { name: "Thomas Wright", url: "https://www.linkedin.com/in/thomas-w-0738232bb/", course: "BSc Computer Science" },
    { name: "Adam Khattab", url: "https://www.linkedin.com/in/adamkhattab/", course: "BSc Computer Science" }
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
            <a className="hover:text-blue-600 transition" href="#video">Sequence</a>
          </nav>
          
          <div className="flex items-center gap-2 sm:gap-3">
            {/* Presentation Toggler with Ping Alert Badge */}
            <button
              type="button"
              onClick={() => setIsPresentationMode(true)}
              className="inline-flex items-center gap-2 rounded-full border border-blue-200 bg-blue-50/80 px-4 py-2 text-sm font-semibold text-blue-700 shadow-sm transition hover:border-blue-400 hover:bg-blue-600 hover:text-white dark:border-blue-900/60 dark:bg-blue-950/40 dark:text-blue-400 dark:hover:bg-blue-500 dark:hover:text-white relative"
              aria-label="Enter Presentation Mode"
            >
              <span className="absolute -top-1 -right-1 flex h-2.5 w-2.5">
                <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-green-400 opacity-75"></span>
                <span className="relative inline-flex rounded-full h-2.5 w-2.5 bg-green-500"></span>
              </span>
              <svg viewBox="0 0 24 24" className="h-4 w-4" fill="none" stroke="currentColor" strokeWidth="2.5" role="img" aria-hidden="true">
                <rect x="2" y="3" width="20" height="14" rx="2" ry="2" />
                <line x1="8" y1="21" x2="16" y2="21" />
                <line x1="12" y1="17" x2="12" y2="21" />
              </svg>
              <span className="hidden sm:inline">Fest Mode</span>
            </button>

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
        </div>
      </header>

      <main className="mx-auto w-full max-w-6xl px-6 pb-24 pt-12">
        
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
          <JunctionSimulation isDark={isDark} />
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

        {/* PROJECT VIDEO SECTION */}
        <section id="video" className="mt-24">
          <div className="text-center max-w-2xl mx-auto space-y-4 mb-10">
            <p className="text-sm font-semibold uppercase tracking-[0.3em] text-blue-600">Sequence</p>
            <h2 className="text-3xl font-semibold">10 Seconds Later</h2>
            <p className="text-slate-600 dark:text-slate-300">
              See the before and after frames showing how the scene changes over 10 seconds. Click on either image to zoom.
            </p>
          </div>
          <div className="relative w-full max-w-6xl mx-auto bg-slate-900 rounded-3xl overflow-hidden shadow-2xl border border-slate-800 p-6 md:p-8">
            <div className="grid gap-6 items-center md:grid-cols-[1.1fr_auto_1.1fr]">
              <figure 
                className="space-y-3 text-center group cursor-zoom-in"
                onClick={() => setActiveLightboxImg({ src: '/2.png', alt: 'Danger: Steering wheel is vibrating', caption: 'Initial Warning: Steering wheel is vibrating' })}
              >
                <img
                  src="/2.png"
                  alt="Frame 2"
                  className="w-full h-auto rounded-2xl border border-slate-800/80 transition-all duration-300 group-hover:scale-[1.02] group-hover:border-blue-500/50 shadow-md group-hover:shadow-blue-500/10"
                />
                <figcaption className="text-xs font-semibold uppercase tracking-[0.2em] text-slate-400 group-hover:text-blue-400 transition duration-300">
                  Danger: Steering wheel is vibrating
                </figcaption>
              </figure>
              <div className="flex flex-col items-center justify-center gap-3 text-center py-4">
                <span className="text-[10px] font-bold uppercase tracking-[0.25em] text-blue-400/90 animate-pulse">10s Transition</span>
                <svg viewBox="0 0 120 24" className="h-6 w-24 text-blue-400 animate-pulse" fill="none" stroke="currentColor" strokeWidth="3.5" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
                  <path d="M2 12h100" />
                  <path d="M94 6l18 6-18 6" />
                </svg>
              </div>
              <figure 
                className="space-y-3 text-center group cursor-zoom-in"
                onClick={() => setActiveLightboxImg({ src: '/1.png', alt: 'Safety: Cyclist passes safely', caption: 'Safe pass completed: Collision avoided' })}
              >
                <img
                  src="/1.png"
                  alt="Frame 1"
                  className="w-full h-auto rounded-2xl border border-slate-800/80 transition-all duration-300 group-hover:scale-[1.02] group-hover:border-blue-500/50 shadow-md group-hover:shadow-blue-500/10"
                />
                <figcaption className="text-xs font-semibold uppercase tracking-[0.2em] text-slate-400 group-hover:text-blue-400 transition duration-300">
                  Safety: Cyclist passes safely
                </figcaption>
              </figure>
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
                    <p className="text-xs text-slate-500 dark:text-slate-400">{member.course}</p>
                  </div>
                  <div className="flex gap-2">
                    {member.url && (
                      <a className="text-xs font-semibold text-blue-600 hover:underline dark:text-blue-400" href={member.url} target="_blank" rel="noreferrer">LinkedIn</a>
                    )}
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

      {/* Presentation HUD Fullscreen Overlay */}
      {isPresentationMode && (
        <div className={`fixed inset-0 z-50 flex flex-col justify-between overflow-hidden transition-colors duration-500 ${isDark ? 'bg-ambient-gradient text-slate-100' : 'bg-ambient-gradient-light text-slate-900'}`}>
          
          {/* Top HUD Bar */}
          {showControls && (
            <div className={`flex w-full items-center justify-between px-8 py-4 border-b ${isDark ? 'border-slate-800/60 bg-slate-950/40' : 'border-slate-200/60 bg-white/40'} backdrop-blur-md z-10`}>
            <div className="flex items-center gap-3">
              <div className="flex h-9 w-9 items-center justify-center rounded-xl bg-blue-600 text-white shadow-lg shadow-blue-500/30">
                <span className="text-md font-bold">V</span>
              </div>
              <div className="text-left">
                <div className="flex items-center gap-2">
                  <p className="text-xs font-semibold uppercase tracking-[0.2em] text-blue-700 dark:text-blue-400">
                    VICAM EXHIBIT
                  </p>
                  <span className="flex h-2 w-2 items-center justify-center relative">
                    <span className="animate-pulse-glow absolute inline-flex h-2.5 w-2.5 rounded-full bg-green-500"></span>
                  </span>
                </div>
                <p className="text-[10px] text-slate-500 dark:text-slate-400 font-medium">Innovation Fest 2026 Showcase</p>
              </div>
            </div>

            {/* Middle Indicator */}
            <div className="hidden md:flex items-center gap-1.5 text-xs font-semibold tracking-wider uppercase text-slate-550 dark:text-slate-400">
              {slides.map((s, idx) => (
                <div key={s.id} className="flex items-center gap-1.5">
                  <button
                    onClick={() => handleJumpToSlide(idx)}
                    className={`transition duration-300 px-3 py-1.5 rounded-lg ${activeSlide === idx ? (isDark ? 'bg-blue-600/25 text-blue-400 border border-blue-500/30' : 'bg-blue-50 text-blue-600 border border-blue-200') : 'hover:text-blue-500 dark:hover:text-blue-400'}`}
                  >
                    {s.label}
                  </button>
                  {idx < slides.length - 1 && <span className="opacity-30">/</span>}
                </div>
              ))}
            </div>

            {/* Right Buttons: Theme toggle & Exit */}
            <div className="flex items-center gap-3">
              <button
                type="button"
                onClick={() => setIsDark((value) => !value)}
                className={`p-2 rounded-full border transition ${isDark ? 'border-slate-800 bg-slate-900 text-slate-300 hover:text-white' : 'border-slate-200 bg-white text-slate-600 hover:text-black'}`}
                aria-label="Toggle theme"
              >
                {isDark ? (
                  <svg viewBox="0 0 24 24" className="h-4 w-4" fill="currentColor">
                    <path d="M12.8 3.2a1 1 0 0 1 1.1 1.5A7 7 0 1 0 19.3 14a1 1 0 0 1 1.5 1.1A9 9 0 1 1 12.8 3.2z" />
                  </svg>
                ) : (
                  <svg viewBox="0 0 24 24" className="h-4 w-4" fill="currentColor">
                    <path d="M12 5a1 1 0 0 1 1 1v1a1 1 0 1 1-2 0V6a1 1 0 0 1 1-1zm0 10a3 3 0 1 0 0-6 3 3 0 0 0 0 6zm7-4a1 1 0 0 1 1 1v1a1 1 0 1 1-2 0v-1a1 1 0 0 1 1-1zM6 12a1 1 0 0 1-1 1H4a1 1 0 1 1 0-2h1a1 1 0 0 1 1 1zm13.07-5.07a1 1 0 0 1 0 1.41l-.7.7a1 1 0 1 1-1.42-1.41l.71-.7a1 1 0 0 1 1.41 0zM7.05 16.95a1 1 0 0 1 0 1.41l-.7.7a1 1 0 1 1-1.42-1.41l.71-.7a1 1 0 0 1 1.41 0zM19.07 16.95a1 1 0 0 1 1.41 0l.7.7a1 1 0 1 1-1.41 1.41l-.7-.7a1 1 0 0 1 0-1.41zM7.05 7.05a1 1 0 0 1-1.41 0l-.7-.7a1 1 0 0 1 1.41-1.41l.7.7a1 1 0 0 1 0 1.41z" />
                  </svg>
                )}
              </button>

              <button
                type="button"
                onClick={() => setIsPresentationMode(false)}
                className="flex items-center gap-2 rounded-xl bg-red-600 px-4 py-2 text-xs font-semibold text-white shadow-lg shadow-red-500/20 transition hover:bg-red-500 hover:shadow-red-500/30"
              >
                <span>Exit</span>
                <kbd className="hidden sm:inline bg-red-700 px-1.5 py-0.5 rounded text-[10px] text-red-200 font-mono">ESC</kbd>
              </button>
            </div>
          </div>
          )}

          {/* Main Slide Container */}
          <div className="flex-1 w-full overflow-y-auto no-scrollbar flex items-center justify-center p-6 md:p-12 z-0">
            <div className="w-full max-w-6xl mx-auto">
              
              {/* Slide 0: Intro */}
              {activeSlide === 0 && (
                <div className="animate-slide-in-right grid items-center gap-12 lg:grid-cols-[1.2fr_0.8fr] py-4">
                  <div className="space-y-6 text-left">
                    <span className="inline-flex w-fit items-center gap-2 rounded-full bg-blue-100 px-4 py-1 text-xs font-semibold uppercase tracking-[0.3em] text-blue-700 dark:bg-blue-500/15 dark:text-blue-300">
                      Innovation Fest 2026
                    </span>
                    <h1 className="text-4xl font-extrabold leading-tight tracking-tight sm:text-6xl text-slate-900 dark:text-white">
                      Spot Cyclists. <br />
                      <span className="text-blue-600 dark:text-blue-400 filter drop-shadow-[0_0_15px_rgba(59,130,246,0.35)]">Before You See Them.</span>
                    </h1>
                    <p className="text-lg md:text-xl text-slate-600 dark:text-slate-300 leading-relaxed font-medium">
                      VICAM protects cyclists at blind corners. Smart sensors talk to each other, giving drivers a soft steering wheel vibration when a bike is near. Safe, silent, and early.
                    </p>
                  </div>

                  <div className="relative">
                    <div className="absolute -top-10 right-0 hidden h-36 w-36 rounded-full bg-blue-400/20 blur-3xl lg:block"></div>
                    <div className="grid gap-4 sm:grid-cols-2">
                      {highlights.map((item) => (
                        <div
                          key={item.label}
                          className={`rounded-2xl border p-6 shadow-md transition-all duration-300 hover:scale-[1.02] ${isDark ? 'border-slate-800 bg-slate-900/60 hover:border-blue-500/50 hover:shadow-blue-900/10' : 'border-slate-200 bg-white hover:border-blue-300 hover:shadow-blue-100'}`}
                        >
                          <p className="text-2xl font-black text-blue-600 dark:text-blue-400">{item.label}</p>
                          <p className="text-sm font-semibold text-slate-500 dark:text-slate-400 mt-2">{item.detail}</p>
                        </div>
                      ))}
                    </div>
                  </div>
                </div>
              )}

              {/* Slide 1: Problem */}
              {activeSlide === 1 && (
                <div className="animate-slide-in-right max-w-4xl mx-auto py-4 text-center space-y-10">
                  <div className="space-y-4">
                    <span className="inline-flex w-fit items-center gap-2 rounded-full bg-red-100 px-4 py-1 text-xs font-semibold uppercase tracking-[0.3em] text-red-700 dark:bg-red-500/15 dark:text-red-300">
                      The Critical Problem
                    </span>
                    <h2 className="text-4xl font-extrabold sm:text-5xl text-slate-900 dark:text-white">
                      Blind spots cause fatal accidents.
                    </h2>
                    <p className="text-lg md:text-xl text-slate-600 dark:text-slate-300 max-w-2xl mx-auto leading-relaxed">
                      Cameras fail at covered junctions. Loud audible alarms panic drivers, leading to erratic responses. We need a silent, non-intrusive early warning.
                    </p>
                  </div>

                  <div className="grid gap-6 md:grid-cols-2 text-left">
                    <div className={`rounded-3xl border p-8 shadow-sm ${isDark ? 'border-slate-800 bg-slate-900/60' : 'border-slate-200 bg-white'}`}>
                      <div className="h-12 w-12 rounded-2xl bg-red-500/10 flex items-center justify-center text-red-500 mb-6 font-bold text-xl">1</div>
                      <h3 className="text-xl font-bold text-slate-900 dark:text-white">Can't See Through Obstructions</h3>
                      <p className="text-slate-600 dark:text-slate-400 mt-3 font-medium leading-relaxed">
                        Line-of-sight cameras are blind around brick walls, high hedges, vans, and architectural structures.
                      </p>
                    </div>
                    <div className={`rounded-3xl border p-8 shadow-sm ${isDark ? 'border-slate-800 bg-slate-900/60' : 'border-slate-200 bg-white'}`}>
                      <div className="h-12 w-12 rounded-2xl bg-red-500/10 flex items-center justify-center text-red-500 mb-6 font-bold text-xl">2</div>
                      <h3 className="text-xl font-bold text-slate-900 dark:text-white">Late & Startling Reactions</h3>
                      <p className="text-slate-600 dark:text-slate-400 mt-3 font-medium leading-relaxed">
                        When alerts occur too late, drivers startle and stomp the brakes or veer, creating high risk for other road users.
                      </p>
                    </div>
                  </div>
                </div>
              )}

              {/* Slide 2: Solution */}
              {activeSlide === 2 && (
                <div className="animate-slide-in-right max-w-4xl mx-auto py-4 text-center space-y-10">
                  <div className="space-y-4">
                    <span className="inline-flex w-fit items-center gap-2 rounded-full bg-blue-100 px-4 py-1 text-xs font-semibold uppercase tracking-[0.3em] text-blue-700 dark:bg-blue-500/15 dark:text-blue-300">
                      The VICAM Solution
                    </span>
                    <h2 className="text-4xl font-extrabold sm:text-5xl text-slate-900 dark:text-white">
                      Silent, early safety awareness.
                    </h2>
                    <p className="text-lg md:text-xl text-slate-600 dark:text-slate-300 max-w-2xl mx-auto leading-relaxed">
                      The cyclist's device broadcasts a continuous signal. The vehicle module intercepts it, vibrating the steering wheel softly in proportion to proximity.
                    </p>
                  </div>

                  <div className="grid gap-4 sm:grid-cols-2 text-left">
                    {features.map((feature, i) => (
                      <div key={feature.title} className={`rounded-2xl border p-6 shadow-sm flex gap-4 items-start ${isDark ? 'border-slate-800 bg-slate-900/60' : 'border-slate-200 bg-white'}`}>
                        <div className="h-10 w-10 shrink-0 rounded-xl bg-blue-600/10 flex items-center justify-center text-blue-500 font-bold text-sm">
                          {i + 1}
                        </div>
                        <div>
                          <p className="font-bold text-slate-900 dark:text-white text-md">{feature.title}</p>
                          <p className="text-xs text-slate-500 dark:text-slate-400 mt-2 leading-relaxed">{feature.description}</p>
                        </div>
                      </div>
                    ))}
                  </div>
                </div>
              )}

              {/* Slide 3: Live Simulation */}
              {activeSlide === 3 && (
                <div className="animate-slide-in-right max-w-5xl mx-auto py-2 space-y-6">
                  <div className="text-center space-y-2">
                    <span className="inline-flex w-fit items-center gap-2 rounded-full bg-blue-100 px-4 py-1 text-xs font-semibold uppercase tracking-[0.3em] text-blue-700 dark:bg-blue-500/15 dark:text-blue-300">
                      Active Exhibit Simulation
                    </span>
                    <h2 className="text-3xl font-extrabold text-slate-900 dark:text-white">Continuous Signal Propagation</h2>
                    <p className="text-slate-600 dark:text-slate-300 max-w-2xl mx-auto text-sm">
                      Watch signals pass straight through building obstructions. The driver receives early tactile warning inside the steering wheel *before* physical visual contact.
                    </p>
                  </div>

                  <JunctionSimulation isDark={isDark} />
                </div>
              )}

              {/* Slide 4: Poster & Specs */}
              {activeSlide === 4 && (
                <div className="animate-slide-in-right max-w-4xl mx-auto py-4 text-center space-y-8">
                  <div className="space-y-3">
                    <span className="inline-flex w-fit items-center gap-2 rounded-full bg-blue-100 px-4 py-1 text-xs font-semibold uppercase tracking-[0.3em] text-blue-700 dark:bg-blue-500/15 dark:text-blue-300">
                      Technical Validation
                    </span>
                    <h2 className="text-4xl font-extrabold text-slate-900 dark:text-white">Technical Specifications & Poster</h2>
                    <p className="text-lg text-slate-600 dark:text-slate-300 max-w-2xl mx-auto">
                      Review complete latency test results, hardware schematics, antenna designs, and user research.
                    </p>
                  </div>

                  <div className="grid gap-6 md:grid-cols-3 max-w-3xl mx-auto">
                    <div className={`rounded-2xl border p-5 ${isDark ? 'border-slate-800 bg-slate-900/60' : 'border-slate-200 bg-white'}`}>
                      <p className="text-3xl font-black text-blue-600 dark:text-blue-400">2.4 GHz</p>
                      <p className="text-sm font-semibold text-slate-500 dark:text-slate-400 mt-2">Active RF Frequency</p>
                    </div>
                    <div className={`rounded-2xl border p-5 ${isDark ? 'border-slate-800 bg-slate-900/60' : 'border-slate-200 bg-white'}`}>
                      <p className="text-3xl font-black text-blue-600 dark:text-blue-400">&lt; 15ms</p>
                      <p className="text-sm font-semibold text-slate-500 dark:text-slate-400 mt-2">End-to-End Latency</p>
                    </div>
                    <div className={`rounded-2xl border p-5 ${isDark ? 'border-slate-800 bg-slate-900/60' : 'border-slate-200 bg-white'}`}>
                      <p className="text-3xl font-black text-blue-600 dark:text-blue-400">99.2%</p>
                      <p className="text-sm font-semibold text-slate-500 dark:text-slate-400 mt-2">Simulated Accuracy</p>
                    </div>
                  </div>

                  <div className={`max-w-2xl mx-auto rounded-3xl border p-8 shadow-sm flex flex-col md:flex-row items-center justify-between gap-6 text-left ${isDark ? 'border-slate-800 bg-slate-900/40' : 'border-slate-200 bg-white'}`}>
                    <div>
                      <h4 className="text-lg font-bold text-slate-900 dark:text-white">VICAM Poster A1</h4>
                      <p className="text-xs text-slate-500 dark:text-slate-400 mt-1 max-w-md">
                        Our comprehensive scientific poster covers target audience research, mechanical mounts, PCB layout, and extensive threat modeling.
                      </p>
                    </div>
                    <a
                      className="rounded-full bg-blue-600 px-6 py-3 text-sm font-bold text-white shadow-lg shadow-blue-500/20 transition hover:bg-blue-500 whitespace-nowrap"
                      href="/VICAM_Poster_A1.pdf"
                      target="_blank"
                      rel="noreferrer"
                    >
                      Open Poster PDF
                    </a>
                  </div>
                </div>
              )}
                   {/* Slide 5: Project Video */}
              {activeSlide === 5 && (
                <div className="animate-slide-in-right max-w-6xl mx-auto py-4 text-center space-y-8">
                  <div className="space-y-3">
                    <span className="inline-flex w-fit items-center gap-2 rounded-full bg-blue-100 px-4 py-1 text-xs font-semibold uppercase tracking-[0.3em] text-blue-700 dark:bg-blue-500/15 dark:text-blue-300">
                      Sequence
                    </span>
                    <h2 className="text-4xl font-extrabold text-slate-900 dark:text-white">10 Seconds Later</h2>
                    <p className="text-lg text-slate-600 dark:text-slate-300 max-w-xl mx-auto">
                      Compare the scene before and after 10 seconds. Click on either image to zoom.
                    </p>
                  </div>

                  <div className="relative w-full max-w-6xl mx-auto bg-slate-900 rounded-3xl overflow-hidden shadow-2xl border border-slate-800 p-6 md:p-8">
                    <div className="grid gap-6 items-center md:grid-cols-[1.1fr_auto_1.1fr]">
                      <figure 
                        className="space-y-3 text-center group cursor-zoom-in"
                        onClick={() => setActiveLightboxImg({ src: '/2.png', alt: 'Danger: Steering wheel is vibrating', caption: 'Initial Warning: Steering wheel is vibrating' })}
                      >
                        <img
                          src="/2.png"
                          alt="Frame 2"
                          className="w-full h-auto rounded-2xl border border-slate-800/80 transition-all duration-300 group-hover:scale-[1.02] group-hover:border-blue-500/50 shadow-md group-hover:shadow-blue-500/10"
                        />
                        <figcaption className="text-xs font-semibold uppercase tracking-[0.2em] text-slate-300 group-hover:text-blue-400 transition duration-300">
                          Danger: Steering wheel is vibrating
                        </figcaption>
                      </figure>
                      <div className="flex flex-col items-center justify-center gap-3 text-center py-4">
                        <span className="text-[10px] font-bold uppercase tracking-[0.25em] text-blue-300/90 animate-pulse">10s Transition</span>
                        <svg viewBox="0 0 120 24" className="h-6 w-24 text-blue-300 animate-pulse" fill="none" stroke="currentColor" strokeWidth="3.5" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
                          <path d="M2 12h100" />
                          <path d="M94 6l18 6-18 6" />
                        </svg>
                      </div>
                      <figure 
                        className="space-y-3 text-center group cursor-zoom-in"
                        onClick={() => setActiveLightboxImg({ src: '/1.png', alt: 'Safety: Cyclist passes safely', caption: 'Safe pass completed: Collision avoided' })}
                      >
                        <img
                          src="/1.png"
                          alt="Frame 1"
                          className="w-full h-auto rounded-2xl border border-slate-800/80 transition-all duration-300 group-hover:scale-[1.02] group-hover:border-blue-500/50 shadow-md group-hover:shadow-blue-500/10"
                        />
                        <figcaption className="text-xs font-semibold uppercase tracking-[0.2em] text-slate-300 group-hover:text-blue-400 transition duration-300">
                          Safety: Cyclist passes safely
                        </figcaption>
                      </figure>
                    </div>
                  </div>
                </div>
              )}

              {/* Slide 6: Team */}
              {activeSlide === 6 && (
                <div className="animate-slide-in-right max-w-4xl mx-auto py-4 text-center space-y-8">
                  <div className="space-y-3">
                    <span className="inline-flex w-fit items-center gap-2 rounded-full bg-blue-100 px-4 py-1 text-xs font-semibold uppercase tracking-[0.3em] text-blue-700 dark:bg-blue-500/15 dark:text-blue-300">
                      The Creators
                    </span>
                    <h2 className="text-4xl font-extrabold text-slate-900 dark:text-white">Meet the VICAM Team</h2>
                    <p className="text-lg text-slate-600 dark:text-slate-300 max-w-xl mx-auto">
                      A multidisciplinary group of engineers committed to making streets safer through hardware and UX innovation.
                    </p>
                  </div>

                  <div className="grid gap-4 sm:grid-cols-2 md:grid-cols-3 max-w-3xl mx-auto justify-center">
                    {team.map((member) => (
                      <div
                        key={member.name}
                        className={`p-5 rounded-2xl border text-center transition hover:scale-[1.02] ${isDark ? 'border-slate-800 bg-slate-900/60' : 'border-slate-200 bg-white'}`}
                      >
                        <div className="h-10 w-10 mx-auto rounded-full bg-blue-600/10 flex items-center justify-center text-blue-600 mb-3 font-bold text-sm">
                          {member.name.charAt(0)}
                        </div>
                        <p className="font-bold text-sm text-slate-900 dark:text-white">{member.name}</p>
                        <p className="text-xs text-slate-500 dark:text-slate-400 mt-1 font-medium">{member.course}</p>
                        {member.url && (
                          <a
                            className="inline-flex items-center gap-1 text-[11px] font-semibold text-blue-600 hover:underline dark:text-blue-400 mt-3"
                            href={member.url}
                            target="_blank"
                            rel="noreferrer"
                          >
                            LinkedIn
                          </a>
                        )}
                      </div>
                    ))}
                  </div>
                </div>
              )}

            </div>
          </div>
          {/* Bottom HUD Control Panel */}
          {showControls && (
            <div className="w-full max-w-5xl mx-auto px-6 pb-8 pt-4 z-10">
              
              {/* Countdown Progress Line */}
              <div className="w-full h-1.5 bg-slate-200/20 dark:bg-slate-850/30 rounded-full overflow-hidden mb-6">
                <div
                  className="h-full bg-blue-500 transition-all ease-linear duration-100"
                  style={{ width: `${progress}%` }}
                ></div>
              </div>

              <div className={`hud-card rounded-3xl p-4 flex flex-col md:flex-row items-center justify-between gap-4 border shadow-2xl ${isDark ? 'hud-glass' : 'hud-glass-light'}`}>
                
                {/* Play/Pause & Nav Arrows */}
                <div className="flex items-center gap-3">
                  <button
                    onClick={handlePrevSlide}
                    className={`p-2.5 rounded-xl transition ${isDark ? 'bg-slate-900 hover:bg-slate-800 text-slate-300 border border-slate-800' : 'bg-slate-100 hover:bg-slate-200 text-slate-700 border border-slate-200'}`}
                    aria-label="Previous Slide"
                  >
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={3} stroke="currentColor" className="w-4 h-4">
                      <path strokeLinecap="round" strokeLinejoin="round" d="M15.75 19.5 8.25 12l7.5-7.5" />
                    </svg>
                  </button>

                  <button
                    onClick={() => setIsPlaying(!isPlaying)}
                    className="p-3 rounded-xl bg-blue-600 text-white transition hover:bg-blue-500 shadow-md shadow-blue-500/25 flex items-center justify-center"
                    aria-label={isPlaying ? 'Pause auto-rotation' : 'Play auto-rotation'}
                  >
                    {isPlaying ? (
                      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" className="w-4 h-4">
                        <path fillRule="evenodd" d="M6.75 5.25a.75.75 0 0 1 .75-.75H9a.75.75 0 0 1 .75.75v13.5a.75.75 0 0 1-.75.75H7.5a.75.75 0 0 1-.75-.75V5.25Zm7.5 0A.75.75 0 0 1 15 4.5h1.5a.75.75 0 0 1 .75.75v13.5a.75.75 0 0 1-.75.75H15a.75.75 0 0 1-.75-.75V5.25Z" clipRule="evenodd" />
                      </svg>
                    ) : (
                      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" className="w-4 h-4">
                        <path fillRule="evenodd" d="M4.5 5.653c0-1.427 1.529-2.33 2.779-1.643l11.54 6.347c1.295.712 1.295 2.573 0 3.286L7.28 19.99c-1.25.687-2.779-.217-2.779-1.643V5.653Z" clipRule="evenodd" />
                      </svg>
                    )}
                  </button>

                  <button
                    onClick={handleNextSlide}
                    className={`p-2.5 rounded-xl transition ${isDark ? 'bg-slate-900 hover:bg-slate-800 text-slate-300 border border-slate-800' : 'bg-slate-100 hover:bg-slate-200 text-slate-700 border border-slate-200'}`}
                    aria-label="Next Slide"
                  >
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={3} stroke="currentColor" className="w-4 h-4">
                      <path strokeLinecap="round" strokeLinejoin="round" d="m8.25 4.5 7.5 7.5-7.5 7.5" />
                    </svg>
                  </button>

                  {/* Keyboard tip */}
                  <span className="hidden lg:inline text-[10px] uppercase font-semibold text-slate-400 dark:text-slate-500 tracking-wider pl-2">
                    Tip: Use [Space], [Arrows], or [H] to hide
                  </span>
                </div>

                {/* Centered slide indicator dots */}
                <div className="flex gap-2">
                  {slides.map((s, idx) => (
                    <button
                      key={s.id}
                      onClick={() => handleJumpToSlide(idx)}
                      className={`h-2.5 rounded-full transition-all duration-300 ${activeSlide === idx ? 'w-8 bg-blue-600' : `w-2.5 ${isDark ? 'bg-slate-900 hover:bg-slate-750 border border-slate-800' : 'bg-slate-300 hover:bg-slate-400 border border-slate-200'}`}`}
                      aria-label={`Jump to slide ${s.label}`}
                    />
                  ))}
                </div>

                {/* Speed & Control Panel Actions */}
                <div className="flex items-center gap-3">
                  <div className="flex items-center gap-2">
                    <span className="text-[11px] font-bold uppercase tracking-wider text-slate-500 dark:text-slate-400">
                      Speed:
                    </span>
                    <div className="flex bg-slate-200/50 dark:bg-slate-900/60 p-0.5 rounded-xl border border-slate-300/40 dark:border-slate-800/50">
                      {[
                        { val: 5, lbl: '5s' },
                        { val: 8, lbl: '8s' },
                        { val: 12, lbl: '12s' },
                      ].map((speed) => (
                        <button
                          key={speed.val}
                          onClick={() => {
                            setSlideDuration(speed.val)
                            setProgress(0)
                          }}
                          className={`px-3 py-1 rounded-lg text-xs font-bold transition duration-300 ${slideDuration === speed.val ? 'bg-blue-600 text-white shadow-md' : 'text-slate-650 dark:text-slate-455 hover:text-blue-500'}`}
                        >
                          {speed.lbl}
                        </button>
                      ))}
                    </div>
                  </div>

                  <button
                    type="button"
                    onClick={() => setShowControls(false)}
                    className={`p-2.5 rounded-xl transition flex items-center justify-center gap-1.5 text-xs font-bold ${isDark ? 'bg-slate-900 hover:bg-slate-800 text-slate-300 border border-slate-800' : 'bg-slate-100 hover:bg-slate-200 text-slate-700 border border-slate-200'}`}
                    title="Hide HUD Controls"
                    aria-label="Hide Controls"
                  >
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={2} stroke="currentColor" className="w-4 h-4">
                      <path strokeLinecap="round" strokeLinejoin="round" d="M3.98 8.223A10.477 10.477 0 0 0 1.934 12C3.226 16.338 7.244 19.5 12 19.5c.993 0 1.953-.138 2.863-.395M6.228 6.228A10.451 10.451 0 0 1 12 4.5c4.756 0 8.773 3.162 10.065 7.498a10.522 10.522 0 0 1-4.293 5.774M6.228 6.228 3 3m3.228 3.228 3.65 3.65m7.815 7.815 3 3m-3-3a10.485 10.485 0 0 1-5.113 1.302m0 0a8.944 8.944 0 0 1-4.293-1.096m0 0L15 12.354" />
                    </svg>
                    <span className="hidden sm:inline">Hide HUD</span>
                  </button>
                </div>

              </div>
            </div>
          )}

          {/* Floating Show Controls Button when HUD is hidden */}
          {!showControls && (
            <button
              type="button"
              onClick={() => setShowControls(true)}
              className="fixed bottom-6 right-6 z-50 p-3.5 rounded-full bg-blue-600 hover:bg-blue-500 text-white shadow-lg shadow-blue-500/30 transition-all hover:scale-110 animate-bounce flex items-center justify-center group"
              title="Show Controls (Press H)"
              aria-label="Show Controls"
            >
              <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={2} stroke="currentColor" className="w-5 h-5">
                <path strokeLinecap="round" strokeLinejoin="round" d="M2.036 12.322a1.012 1.012 0 0 1 0-.639C3.423 7.51 7.36 4.5 12 4.5c4.638 0 8.573 3.007 9.963 7.178.07.207.07.431 0 .639C20.577 16.49 16.64 19.5 12 19.5c-4.638 0-8.573-3.007-9.963-7.178Z" />
                <circle cx="12" cy="12" r="3" />
              </svg>
              <span className="max-w-0 overflow-hidden whitespace-nowrap group-hover:max-w-xs transition-all duration-300 ease-out font-bold text-xs pl-0 group-hover:pl-2">
                Show HUD
              </span>
            </button>
          )}

        </div>
      )}

      {/* Lightbox Modal */}
      {activeLightboxImg && (
        <div 
          className="fixed inset-0 z-[100] flex flex-col items-center justify-center bg-black/90 p-4 md:p-8 backdrop-blur-md cursor-zoom-out animate-fade-in"
          onClick={() => setActiveLightboxImg(null)}
        >
          <button 
            onClick={() => setActiveLightboxImg(null)}
            className="absolute top-6 right-6 text-white/60 hover:text-white bg-white/10 hover:bg-white/20 p-3 rounded-full transition duration-300"
            aria-label="Close image preview"
          >
            <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={2.5} stroke="currentColor" className="w-6 h-6">
              <path strokeLinecap="round" strokeLinejoin="round" d="M6 18 18 6M6 6l12 12" />
            </svg>
          </button>
          
          <div className="relative max-w-5xl w-full max-h-[85vh] flex flex-col items-center justify-center px-4" onClick={(e) => e.stopPropagation()}>
            <img 
              src={activeLightboxImg.src} 
              alt={activeLightboxImg.alt} 
              className="max-w-full max-h-[75vh] object-contain rounded-2xl shadow-2xl border border-white/15 animate-scale-up"
            />
            {activeLightboxImg.caption && (
              <p className="mt-4 text-xs md:text-sm font-semibold tracking-widest uppercase text-white/90 bg-slate-900/90 backdrop-blur-md px-5 py-2.5 rounded-xl border border-slate-800/80 shadow-lg">
                {activeLightboxImg.caption}
              </p>
            )}
          </div>
        </div>
      )}
    </div>
  )
}

export default App
