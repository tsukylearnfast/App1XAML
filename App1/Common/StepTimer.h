#pragma once

#include <wrl.h>

namespace DX
{
	// Classe d’assistance pour le minutage de l’animation et de la simulation.
	class StepTimer
	{
	public:
		StepTimer() : 
			m_elapsedTicks(0),
			m_totalTicks(0),
			m_leftOverTicks(0),
			m_frameCount(0),
			m_framesPerSecond(0),
			m_framesThisSecond(0),
			m_qpcSecondCounter(0),
			m_isFixedTimeStep(false),
			m_targetElapsedTicks(TicksPerSecond / 60)
		{
			if (!QueryPerformanceFrequency(&m_qpcFrequency))
			{
				throw ref new Platform::FailureException();
			}

			if (!QueryPerformanceCounter(&m_qpcLastTime))
			{
				throw ref new Platform::FailureException();
			}

			// Initialiser le délai max. à 1/10 de seconde.
			m_qpcMaxDelta = m_qpcFrequency.QuadPart / 10;
		}

		// Obtenez le temps écoulé depuis l'appel précédent de Update.
		uint64 GetElapsedTicks() const						{ return m_elapsedTicks; }
		double GetElapsedSeconds() const					{ return TicksToSeconds(m_elapsedTicks); }

		// Obtenir le temps total écoulé depuis le début du programme.
		uint64 GetTotalTicks() const						{ return m_totalTicks; }
		double GetTotalSeconds() const						{ return TicksToSeconds(m_totalTicks); }

		// Obtenez le nombre total de mises à jour depuis le début du programme.
		uint32 GetFrameCount() const						{ return m_frameCount; }

		// Obtenir le taux de trames actuel.
		uint32 GetFramesPerSecond() const					{ return m_framesPerSecond; }

		// Définir si le mode timestep fixe ou variable doit être utilisé.
		void SetFixedTimeStep(bool isFixedTimestep)			{ m_isFixedTimeStep = isFixedTimestep; }

		// Définissez la fréquence d'appel de Update en mode timestep fixe.
		void SetTargetElapsedTicks(uint64 targetElapsed)	{ m_targetElapsedTicks = targetElapsed; }
		void SetTargetElapsedSeconds(double targetElapsed)	{ m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

		// Le format entier représente l’heure à l’aide de 10 000 000 battements par seconde.
		static const uint64 TicksPerSecond = 10000000;

		static double TicksToSeconds(uint64 ticks)			{ return static_cast<double>(ticks) / TicksPerSecond; }
		static uint64 SecondsToTicks(double seconds)		{ return static_cast<uint64>(seconds * TicksPerSecond); }

		// Après une interruption intentionnelle du minutage (par exemple une opération E/S bloquante)
		// appeler cela pour éviter que la logique timestep fixe tente un ensemble d’appels de rattrapage 
		// Appels de Update.

		void ResetElapsedTime()
		{
			if (!QueryPerformanceCounter(&m_qpcLastTime))
			{
				throw ref new Platform::FailureException();
			}

			m_leftOverTicks = 0;
			m_framesPerSecond = 0;
			m_framesThisSecond = 0;
			m_qpcSecondCounter = 0;
		}

		// Mettez à jour l'état de la minuterie, en appelant la fonction de mise à jour spécifiée le nombre de fois approprié.
		template<typename TUpdate>
		void Tick(const TUpdate& update)
		{
			// Interroger l’heure actuelle.
			LARGE_INTEGER currentTime;

			if (!QueryPerformanceCounter(&currentTime))
			{
				throw ref new Platform::FailureException();
			}

			uint64 timeDelta = currentTime.QuadPart - m_qpcLastTime.QuadPart;

			m_qpcLastTime = currentTime;
			m_qpcSecondCounter += timeDelta;

			// Limiter les délais trop longs (par exemple après une pause dans le débogueur).
			if (timeDelta > m_qpcMaxDelta)
			{
				timeDelta = m_qpcMaxDelta;
			}

			// Convertissez les unités QPC dans un format de battement canonique. Dépassement de capacité impossible à cause de la limite précédente.
			timeDelta *= TicksPerSecond;
			timeDelta /= m_qpcFrequency.QuadPart;

			uint32 lastFrameCount = m_frameCount;

			if (m_isFixedTimeStep)
			{
				// Logique de mise à jour timestep fixe

				// Si l’application est très proche du temps écoulé cible (dans les 1/4 de milliseconde), fixer simplement une limite
				// à l’horloge pour correspondre exactement à la valeur cible. Cela évite les petites erreurs inutiles
				// d'accumulation dans le temps. Sans cette restriction, un jeu qui exigeait 60 fps
				// mise à jour fixe, exécutée avec vsync activé sur un écran NTSC 59.94, entraînerait
				// accumuler assez de petites erreurs pour supprimer un frame. Il est préférable d'arrondir 
				// les petits écarts à zéro pour permettre une exécution sans problème.

				if (abs(static_cast<int64>(timeDelta - m_targetElapsedTicks)) < TicksPerSecond / 4000)
				{
					timeDelta = m_targetElapsedTicks;
				}

				m_leftOverTicks += timeDelta;

				while (m_leftOverTicks >= m_targetElapsedTicks)
				{
					m_elapsedTicks = m_targetElapsedTicks;
					m_totalTicks += m_targetElapsedTicks;
					m_leftOverTicks -= m_targetElapsedTicks;
					m_frameCount++;

					update();
				}
			}
			else
			{
				// Logique de mise à jour timestep variable.
				m_elapsedTicks = timeDelta;
				m_totalTicks += timeDelta;
				m_leftOverTicks = 0;
				m_frameCount++;

				update();
			}

			// Effectuer le suivi du taux de trames actuel.
			if (m_frameCount != lastFrameCount)
			{
				m_framesThisSecond++;
			}

			if (m_qpcSecondCounter >= static_cast<uint64>(m_qpcFrequency.QuadPart))
			{
				m_framesPerSecond = m_framesThisSecond;
				m_framesThisSecond = 0;
				m_qpcSecondCounter %= m_qpcFrequency.QuadPart;
			}
		}

	private:
		// Les données de temporisation source utilisent des unités QPC.
		LARGE_INTEGER m_qpcFrequency;
		LARGE_INTEGER m_qpcLastTime;
		uint64 m_qpcMaxDelta;

		// Les données de temporisation dérivées utilisent un format de battement canonique.
		uint64 m_elapsedTicks;
		uint64 m_totalTicks;
		uint64 m_leftOverTicks;

		// Membres destinés au suivi du taux de trames.
		uint32 m_frameCount;
		uint32 m_framesPerSecond;
		uint32 m_framesThisSecond;
		uint64 m_qpcSecondCounter;

		// Membres destinés à configurer le mode timestep fixe.
		bool m_isFixedTimeStep;
		uint64 m_targetElapsedTicks;
	};
}
