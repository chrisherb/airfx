#include "c74_min.h"

using namespace c74::min;

class srsly2 : public object<srsly2>, public vector_operator<> {
public:
	MIN_DESCRIPTION {"psychoacoustic stereo processor"};
	MIN_TAGS {"audio, effect"};
	MIN_AUTHOR {"Isabel Kaspriskie"};

	inlet<> in1 {this, "(signal) Input1"};
	inlet<> in2 {this, "(signal) Input2"};
	outlet<> out1 {this, "(signal) Output1", "signal"};
	outlet<> out2 {this, "(signal) Output2", "signal"};

	attribute<number, threadsafe::no, limit::clamp> A {this, "Center", 0.5, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> B {this, "Space", 0.5, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> C {this, "Level", 1.0, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> D {this, "Q", 0.5, range {0.0, 1.0} };
	attribute<number, threadsafe::no, limit::clamp> E {this, "Dry/Wet", 1.0, range {0.0, 1.0} };

	message<> dspsetup {this, "dspsetup",
		MIN_FUNCTION {
			for (int x = 0; x < 11; x++) {
				biquadM2[x] = 0.0;
				biquadM7[x] = 0.0;
				biquadM10[x] = 0.0;
				biquadL3[x] = 0.0;
				biquadL7[x] = 0.0;
				biquadR3[x] = 0.0;
				biquadR7[x] = 0.0;
				biquadS3[x] = 0.0;
				biquadS5[x] = 0.0;
			}
			A = 0.5;
			B = 0.5;
			C = 1.0;
			D = 0.5;
			E = 1.0;
			fpdL = 1.0; while (fpdL < 16386) fpdL = rand()*UINT32_MAX;
			fpdR = 1.0; while (fpdR < 16386) fpdR = rand()*UINT32_MAX;
			//this is reset: values being initialized only once. Startup values, whatever they are.
			
			return {};
		}
	};

	void operator()(audio_bundle _input, audio_bundle _output) {
		double* in1 = _input.samples(0);
		double* in2 = _input.samples(1);
		double* out1 = _output.samples(0);
		double* out2 = _output.samples(1);
		long sampleFrames = _input.frame_count();

		double sampleRate = samplerate();
		if (sampleRate < 22000) sampleRate = 22000; //keep biquads in range
		long double tempSample;
		
		biquadM2[0] = 2000 / sampleRate; //up
		biquadM7[0] = 7000 / sampleRate; //down
		biquadM10[0] = 10000 / sampleRate; //down
		
		biquadL3[0] = 3000 / sampleRate; //up
		biquadL7[0] = 7000 / sampleRate; //way up
		biquadR3[0] = 3000 / sampleRate; //up
		biquadR7[0] = 7000 / sampleRate; //way up
		
		biquadS3[0] = 3000 / sampleRate; //up
		biquadS5[0] = 5000 / sampleRate; //way down
		
		double focusM = 15.0-(A*10.0);
		double focusS = 21.0-(B*15.0);
		double Q = D+0.25; //add Q control: from half to double intensity
		double gainM = A*2.0;
		double gainS = B*2.0;
		if (gainS > 1.0) gainM /= gainS;
		if (gainM > 1.0) gainM = 1.0;
		
	    biquadM2[1] = focusM*0.25*Q; //Q, mid 2K boost is much broader
	    biquadM7[1] = focusM*Q; //Q
	    biquadM10[1] = focusM*Q; //Q
	    biquadS3[1] = focusM*Q; //Q
	    biquadS5[1] = focusM*Q; //Q
		
	    biquadL3[1] = focusS*Q; //Q
	    biquadL7[1] = focusS*Q; //Q
	    biquadR3[1] = focusS*Q; //Q
	    biquadR7[1] = focusS*Q; //Q
		
		double K = tan(M_PI * biquadM2[0]);
		double norm = 1.0 / (1.0 + K / biquadM2[1] + K * K);
		biquadM2[2] = K / biquadM2[1] * norm;
		biquadM2[4] = -biquadM2[2];
		biquadM2[5] = 2.0 * (K * K - 1.0) * norm;
		biquadM2[6] = (1.0 - K / biquadM2[1] + K * K) * norm;
		
		K = tan(M_PI * biquadM7[0]);
		norm = 1.0 / (1.0 + K / biquadM7[1] + K * K);
		biquadM7[2] = K / biquadM7[1] * norm;
		biquadM7[4] = -biquadM7[2];
		biquadM7[5] = 2.0 * (K * K - 1.0) * norm;
		biquadM7[6] = (1.0 - K / biquadM7[1] + K * K) * norm;
		
		K = tan(M_PI * biquadM10[0]);
		norm = 1.0 / (1.0 + K / biquadM10[1] + K * K);
		biquadM10[2] = K / biquadM10[1] * norm;
		biquadM10[4] = -biquadM10[2];
		biquadM10[5] = 2.0 * (K * K - 1.0) * norm;
		biquadM10[6] = (1.0 - K / biquadM10[1] + K * K) * norm;
		
		K = tan(M_PI * biquadL3[0]);
		norm = 1.0 / (1.0 + K / biquadL3[1] + K * K);
		biquadL3[2] = K / biquadL3[1] * norm;
		biquadL3[4] = -biquadL3[2];
		biquadL3[5] = 2.0 * (K * K - 1.0) * norm;
		biquadL3[6] = (1.0 - K / biquadL3[1] + K * K) * norm;
		
		K = tan(M_PI * biquadL7[0]);
		norm = 1.0 / (1.0 + K / biquadL7[1] + K * K);
		biquadL7[2] = K / biquadL7[1] * norm;
		biquadL7[4] = -biquadL7[2];
		biquadL7[5] = 2.0 * (K * K - 1.0) * norm;
		biquadL7[6] = (1.0 - K / biquadL7[1] + K * K) * norm;
		
		K = tan(M_PI * biquadR3[0]);
		norm = 1.0 / (1.0 + K / biquadR3[1] + K * K);
		biquadR3[2] = K / biquadR3[1] * norm;
		biquadR3[4] = -biquadR3[2];
		biquadR3[5] = 2.0 * (K * K - 1.0) * norm;
		biquadR3[6] = (1.0 - K / biquadR3[1] + K * K) * norm;
		
		K = tan(M_PI * biquadR7[0]);
		norm = 1.0 / (1.0 + K / biquadR7[1] + K * K);
		biquadR7[2] = K / biquadR7[1] * norm;
		biquadR7[4] = -biquadR7[2];
		biquadR7[5] = 2.0 * (K * K - 1.0) * norm;
		biquadR7[6] = (1.0 - K / biquadR7[1] + K * K) * norm;
		
		K = tan(M_PI * biquadS3[0]);
		norm = 1.0 / (1.0 + K / biquadS3[1] + K * K);
		biquadS3[2] = K / biquadS3[1] * norm;
		biquadS3[4] = -biquadS3[2];
		biquadS3[5] = 2.0 * (K * K - 1.0) * norm;
		biquadS3[6] = (1.0 - K / biquadS3[1] + K * K) * norm;
		
		K = tan(M_PI * biquadS5[0]);
		norm = 1.0 / (1.0 + K / biquadS5[1] + K * K);
		biquadS5[2] = K / biquadS5[1] * norm;
		biquadS5[4] = -biquadS5[2];
		biquadS5[5] = 2.0 * (K * K - 1.0) * norm;
		biquadS5[6] = (1.0 - K / biquadS5[1] + K * K) * norm;
		
		double depthM = pow(A,2)*2.0;; //proportion to mix in the filtered stuff
		double depthS = pow(B,2)*2.0;; //proportion to mix in the filtered stuff
		double level = C; //output pad
		double wet = E; //dry/wet
		
		//biquad contains these values:
		//[0] is frequency: 0.000001 to 0.499999 is _near-zero to _near-Nyquist
		//[1] is resonance, 0.7071 is Butterworth. Also can't be zero
		//[2] is a0 but you need distinct ones for additional biquad instances so it's here
		//[3] is a1 but you need distinct ones for additional biquad instances so it's here
		//[4] is a2 but you need distinct ones for additional biquad instances so it's here
		//[5] is b1 but you need distinct ones for additional biquad instances so it's here
		//[6] is b2 but you need distinct ones for additional biquad instances so it's here
		//[7] is LEFT stored delayed sample (freq and res are stored so you can move them sample by sample)
		//[8] is LEFT stored delayed sample (you have to include the coefficient making code if you do that)
		//[9] is RIGHT stored delayed sample (freq and res are stored so you can move them sample by sample)
		//[10] is RIGHT stored delayed sample (you have to include the coefficient making code if you do that)
		
	    while (--sampleFrames >= 0)
	    {
			long double inputSampleL = *in1;
			long double inputSampleR = *in2;
			if (fabs(inputSampleL)<1.18e-43) inputSampleL = fpdL * 1.18e-43;
			if (fabs(inputSampleR)<1.18e-43) inputSampleR = fpdR * 1.18e-43;
			long double drySampleL = inputSampleL;
			long double drySampleR = inputSampleR;
			
			inputSampleL = sin(inputSampleL);
			inputSampleR = sin(inputSampleR);
			//encode Console5: good cleanness
			
			long double mid = inputSampleL + inputSampleR;
			long double rawmid = mid * 0.5; //we'll use this to isolate L&R a little
			long double side = inputSampleL - inputSampleR;
			long double boostside = side * depthS;
			//assign mid and side.Between these sections, you can do mid/side processing
			
			tempSample = (mid * biquadM2[2]) + biquadM2[7];
			biquadM2[7] = (-tempSample * biquadM2[5]) + biquadM2[8];
			biquadM2[8] = (mid * biquadM2[4]) - (tempSample * biquadM2[6]);
			long double M2Sample = tempSample; //like mono AU, 7 and 8 store L channel
			
			tempSample = (mid * biquadM7[2]) + biquadM7[7];
			biquadM7[7] = (-tempSample * biquadM7[5]) + biquadM7[8];
			biquadM7[8] = (mid * biquadM7[4]) - (tempSample * biquadM7[6]);
			long double M7Sample = -tempSample*2.0; //like mono AU, 7 and 8 store L channel
			
			tempSample = (mid * biquadM10[2]) + biquadM10[7];
			biquadM10[7] = (-tempSample * biquadM10[5]) + biquadM10[8];
			biquadM10[8] = (mid * biquadM10[4]) - (tempSample * biquadM10[6]);
			long double M10Sample = -tempSample*2.0; //like mono AU, 7 and 8 store L channel
			//mid
			
			tempSample = (side * biquadS3[2]) + biquadS3[7];
			biquadS3[7] = (-tempSample * biquadS3[5]) + biquadS3[8];
			biquadS3[8] = (side * biquadS3[4]) - (tempSample * biquadS3[6]);
			long double S3Sample = tempSample*2.0; //like mono AU, 7 and 8 store L channel
			
			tempSample = (side * biquadS5[2]) + biquadS5[7];
			biquadS5[7] = (-tempSample * biquadS5[5]) + biquadS5[8];
			biquadS5[8] = (side * biquadS5[4]) - (tempSample * biquadS5[6]);
			long double S5Sample = -tempSample*5.0; //like mono AU, 7 and 8 store L channel
			
			mid = (M2Sample + M7Sample + M10Sample)*depthM;
			side = (S3Sample + S5Sample + boostside)*depthS;
			
			long double msOutSampleL = (mid+side)/2.0;
			long double msOutSampleR = (mid-side)/2.0;
			//unassign mid and side
			
			long double isoSampleL = inputSampleL-rawmid;
			long double isoSampleR = inputSampleR-rawmid; //trying to isolate L and R a little
			
			tempSample = (isoSampleL * biquadL3[2]) + biquadL3[7];
			biquadL3[7] = (-tempSample * biquadL3[5]) + biquadL3[8];
			biquadL3[8] = (isoSampleL * biquadL3[4]) - (tempSample * biquadL3[6]);
			long double L3Sample = tempSample; //like mono AU, 7 and 8 store L channel
			
			tempSample = (isoSampleR * biquadR3[2]) + biquadR3[9];
			biquadR3[9] = (-tempSample * biquadR3[5]) + biquadR3[10];
			biquadR3[10] = (isoSampleR * biquadR3[4]) - (tempSample * biquadR3[6]);
			long double R3Sample = tempSample; //note: 9 and 10 store the R channel
			
			tempSample = (isoSampleL * biquadL7[2]) + biquadL7[7];
			biquadL7[7] = (-tempSample * biquadL7[5]) + biquadL7[8];
			biquadL7[8] = (isoSampleL * biquadL7[4]) - (tempSample * biquadL7[6]);
			long double L7Sample = tempSample*3.0; //like mono AU, 7 and 8 store L channel
			
			tempSample = (isoSampleR * biquadR7[2]) + biquadR7[9];
			biquadR7[9] = (-tempSample * biquadR7[5]) + biquadR7[10];
			biquadR7[10] = (isoSampleR * biquadR7[4]) - (tempSample * biquadR7[6]);
			long double R7Sample = tempSample*3.0; //note: 9 and 10 store the R channel
			
			long double processingL = msOutSampleL + ((L3Sample + L7Sample)*depthS);
			long double processingR = msOutSampleR + ((R3Sample + R7Sample)*depthS);
			//done with making filters, now we apply them
			
			mid = inputSampleL + inputSampleR;
			side = inputSampleL - inputSampleR;
			//re-assign mid and side.Between these sections, you can do mid/side processing
			
			mid *= gainM;
			side *= gainS;
			//we crank things up more than a bit, or cut them in line with how the hardware box works
			if (side > 1.57079633) side = 1.57079633;
			if (side < -1.57079633) side = -1.57079633;
			side = sin(side);
			side *= gainS;
			
			
			inputSampleL = ((mid+side)/2.0)+processingL;
			inputSampleR = ((mid-side)/2.0)+processingR;
			//unassign mid and side
			
			if (level < 1.0) {
				inputSampleL *= level;
				inputSampleR *= level;
			}
			
			if (inputSampleL > 1.0) inputSampleL = 1.0;
			if (inputSampleL < -1.0) inputSampleL = -1.0;
			if (inputSampleR > 1.0) inputSampleR = 1.0;
			if (inputSampleR < -1.0) inputSampleR = -1.0;
			//without this, you can get a NaN condition where it spits out DC offset at full blast!
			inputSampleL = asin(inputSampleL);
			inputSampleR = asin(inputSampleR);
			//amplitude aspect
			
			if (wet < 1.0) {
				inputSampleL = (inputSampleL * wet)+(drySampleL * (1.0-wet));
				inputSampleR = (inputSampleR * wet)+(drySampleR * (1.0-wet));
			}
			
			//begin 64 bit stereo floating point dither
			int expon; frexp((double)inputSampleL, &expon);
			fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
			inputSampleL += ((double(fpdL)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
			frexp((double)inputSampleR, &expon);
			fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
			inputSampleR += ((double(fpdR)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
			//end 64 bit stereo floating point dither
			
			*out1 = inputSampleL;
			*out2 = inputSampleR;
	
			*in1++;
			*in2++;
			*out1++;
			*out2++;
		}
	}
private:
    
    
	long double biquadM2[11];
	long double biquadM7[11];
	long double biquadM10[11];
	
	long double biquadL3[11];
	long double biquadL7[11];
	long double biquadR3[11];
	long double biquadR7[11];
	
	long double biquadS3[11];
	long double biquadS5[11];
	
	uint32_t fpdL;
	uint32_t fpdR;
	//default stuff

};
MIN_EXTERNAL(srsly2);
