#include "ofApp.h"

#define NRECT	63
#define NPART	(NRECT*4)

const int screenW = 1024, screenH = 768;

const int width = 80;
const int height = 80;
const int gapX = 5;
const int gapY = 5;

const int xstart = (screenW - (7 * width)) / 2;
const int ystart = (screenH - (9 * height)) / 2;

const int Cx = (80 * 7) / 2 + xstart;
const int Cy = (80 * 9) / 2 + ystart;


struct part {
	bool active;
	double x, y, theta;
	double dx, dy, dtheta;
	ofColor color;
};
struct part parts[NPART];
int nextPart = 0;

const double gravity = 0.01;

// viewport
int vx = 0, vy = 0, vw = 1024, vh = 768;

#define NPAL	32
ofColor palette[NPAL];

// works like a shift register
int palIndex[NRECT];
#define NPALMODES 3
int palMode = 2;

ofTrueTypeFont font;

//--------------------------------------------------------------
void ofApp::setup(){
	int p = 0;
	palette[p++] = ofColor(255, 255, 255);
	palette[p++] = ofColor(0, 0, 0);
	palette[p++] = ofColor(255, 0, 0);
	palette[p++] = ofColor(0, 255, 0);
	palette[p++] = ofColor(0, 0, 255);
	for (int r = 50; r <= 250; r += 100) {
		for (int g = 50; g <= 250; g += 100) {
			for (int b = 50; b <= 250; b += 100) {
				palette[p++] = ofColor(r, g, b);
			}
		}
	}
	font.load("bauhaus.ttf", 20);
}

//--------------------------------------------------------------
int cx = Cx, cy = Cy;
bool orbit = false;
float theta = 0;
float bumpAmt = 0;
float bumpDelta = 0;

// 0 to 255
float alphaFill = 255;
float alphaFillDelta = 0;
float alphaBorder = 255;
float alphaBorderDelta = 0;

int shatter = 0;
bool borderChase = false;
bool hypnotize = false;
bool showID = false;

float borderPhase = 0.0;
float borderStep = 0.001;

float jitter = 0.0;
float jitterDecay = 0.95;

void ofApp::update(){
	/* update particles */
	if (shatter == 2) {
		for (int i = 0; i < NPART; i++) {
			if (!parts[i].active) continue;
			parts[i].x += parts[i].dx; parts[i].y += parts[i].dy; parts[i].theta += parts[i].dtheta;
			parts[i].dy += gravity;
			if (parts[i].y > 1000)
				parts[i].active = false;
		}
	}

	/* bump */
	bumpAmt = ofClamp(bumpAmt + bumpDelta, 0, 100);
	if (bumpAmt == 100) {
		shatter = 1;
		bumpAmt = 0;
		bumpDelta = 0;
	}
	if (bumpAmt < 0.001) {
		bumpAmt = 0;
		bumpDelta = 0;
	}

	// border cycle
	if (borderChase) {
		borderPhase += borderStep;
		if (borderPhase > 1.0) borderPhase = 0.0;
	}

	// jitter decay
	jitter *= jitterDecay;
	if (jitter < 0.01) jitter = 0;

	// palette shift
	if ((ofGetFrameNum() % 10) == 0) {
		int w = 7;
		switch (palMode) {
		case 0: // linear
			for (int i = NRECT - 1; i > 0; i--) {
				palIndex[i] = palIndex[i - 1];
			}
			break;
		case 1: // ribbons
			for (int i = 0; i < NRECT; i += 2*w) {
				palIndex[i] = palIndex[0];
				for (int j = i + w-1; j > i; j--)
					palIndex[j] = palIndex[j - 1];
			}
			for (int i = w; i < NRECT; i += 2*w) {
				palIndex[i + w-1] = palIndex[w-1];
				for (int j = i; j < i + w-1; j++)
					palIndex[j] = palIndex[j + 1];
			}
			break;
		case 2: // diag
			for (int i = NRECT-w; i > 0; i -= w)
				palIndex[i] = palIndex[i - w];
			for (int i = NRECT; i > 0; i--) {
				if ((i % w) == 0) continue;
				palIndex[i] = palIndex[i - 1];
			}
			break;
		}
	}

	// alpha fades
	alphaFill = ofClamp(alphaFill + alphaFillDelta, 0, 255);
	alphaBorder = ofClamp(alphaBorder + alphaBorderDelta, 0, 255);

	// orbit center point
	theta += 0.005;
	if (orbit) {
		cx = Cx + 100 * sin(theta);
		cy = Cy + 100 * cos(theta);
	}
	else {
		cx = Cx;
		cy = Cy;
	}
}

// "bump" lens distortion
// and random jitter if applicable
void distort(int *x, int *y) {
	const double maxD = 400;

	float D = ofDist(*x, *y, cx, cy);
	if (D < 0) D = D*-1;
	if (D > maxD) D = maxD;
	int dx = *x - cx;
	int dy = *y - cy;
	float scale = (1-(D / maxD)) * (bumpAmt/80);

	*x += dx * scale + ofRandom(-jitter, jitter);
	*y += dy * scale + ofRandom(-jitter, jitter);
}

ofPoint interp(int bx[], int by[], int count, float frac) {
	float index = frac*count;
	int p0 = (int)index;
	int p1 = (p0 + 1) % count;
	frac = index - p0;
	ofPoint p;
	p.x = bx[p0] + (bx[p1] - bx[p0])*frac;
	p.y = by[p0] + (by[p1] - by[p0])*frac;
	return p;
}

void drawRect(int x1, int y1, int x2, int y2, ofColor fillColor) {
	int x[] = { x1, x2, x2, x1 };
	int y[] = { y1, y1, y2, y2 };

	for (int i = 0; i < 4; i++)
		distort(&x[i], &y[i]);

	if (shatter == 1) {
		// break each rect into 4x particles
		for (int px = 0; px < 2; px++) {
			for (int py = 0; py < 2; py++) {
				parts[nextPart].x = x[0] + 20 + px * 20;
				parts[nextPart].y = y[0] + 20 + py * 20;
				parts[nextPart].theta = ofRandom(-1, 1);

				parts[nextPart].dx = ofRandom(-0.1, 0.1);
				parts[nextPart].dy = ofRandom(-0.5, 0);
				parts[nextPart].dtheta = ofRandom(-0.5, 0.5);

				parts[nextPart].color = fillColor;
				parts[nextPart].active = true;
				nextPart++;
			}
		}
		return;
	}

	// fill
	ofSetColor(fillColor, alphaFill);
	ofDrawTriangle(x[0], y[0], x[1], y[1], x[2], y[2]);
	ofDrawTriangle(x[0], y[0], x[2], y[2], x[3], y[3]);

	// border
	ofSetColor(ofColor(255, 255, 255, alphaBorder));

	// chase border
	if (borderChase) {
		int bx[] = { x1, (x1 + x2) / 2, x2, x2, (x1 + x2) / 2, x1 };
		int by[] = { y1, y1, y1, y2, y2, y2 };
		ofPolyline line;

		const float borderPct = 2 / (float)3;
		float borderEnd = borderPhase + borderPct;
		if (borderEnd > 1.0) borderEnd -= 1.0;

		line.addVertex(interp(bx, by, 6, borderPhase));

		int p0 = (int)(borderPhase * 6 + 1) % 6;
		int p1 = borderEnd * 6;
		for (int i = p0; ; i = (i + 1) % 6) {
			line.addVertex(bx[i], by[i]);
			if (i == p1) break;
		}
		line.addVertex(interp(bx, by, 6, borderEnd));
		line.draw();
	}
	else {
		// full border
		for (int i = 0; i < 4; i++) {
			ofDrawLine(x[i], y[i], x[(i + 1) % 4], y[(i + 1) % 4]);
		}
	}

	//hypnotize
	if (hypnotize) {
		for (int d = 2 + (ofGetFrameNum()/20) % 10; d < 20; d += 5) {
			ofSetColor(255 - d*6);
			ofDrawLine(x1 + d, y1 + d / 2, x2 - d, y1 + d / 2);
			ofDrawLine(x2 - d, y1 + d / 2, x2 - d, y2 - d / 2);
			ofDrawLine(x2 - d, y2 - d / 2, x1 + d, y2 - d / 2);
			ofDrawLine(x1 + d, y2 - d / 2, x1 + d, y1 + d / 2);
		}
	}

	// highlight corners
	/*
	ofSetColor(ofColor(0, 255, 0));
	for (int i = 0; i < 4; i++) {
		ofDrawCircle(x[i], y[i], 3);
	}
	*/
}

//--------------------------------------------------------------
bool hide(int x, int y) {
	if (y == 0 && x != 3) return true;
	if (y == 1 && (x < 2 || x > 4)) return true;
	if (y == 2 && (x < 1 || x > 5)) return true;
	return false;
}

void ofApp::draw(){
	ofClear(ofColor(0, 0, 0));
	ofViewport(vx, vy, vw, vh);

	if (shatter < 2) {
		int rIndex = 0;

		for (int x = 0; x < 7; x++) {
			for (int y = 0; y < 9; y++) {
				if (hide(x, y)) {
					rIndex++;
					continue;
				}
				int xx = xstart + x * width;
				int yy = ystart + y * height;

				drawRect(xx, yy, xx + width-gapX, yy + height-gapY, palette[palIndex[rIndex]]);
				
				if (showID) {
					ofSetColor(0, 255, 0);
					font.drawString(to_string(rIndex), xx + 10, yy + 25);
				}
				rIndex++;
			}
		}
		if (shatter == 1) {
			nextPart = 0;
			shatter = 2;
		}
	} else { // particles
		int nActive = 0;
		for (int i = 0; i < NPART; i++) {
			if (!parts[i].active) continue;
			nActive++;
			ofSetColor(parts[i].color);
			ofPushMatrix();
			ofTranslate(parts[i].x, parts[i].y);
			ofRotateZ(parts[i].theta);
			int r = 10;
			ofDrawRectangle(-r, -r, 2 * r, 2 * r);
			ofPopMatrix();
		}
		if (nActive == 0) shatter = 0;
	}

	// mask
	/*
	ofSetColor(0, 0, 0);
	ofDrawRectangle(0, 0, 59, 1000);
	ofDrawRectangle(951, 0, 1000, 1000);
	*/
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
	case ' ': // bump
		bumpAmt = 10;
		bumpDelta = -0.1;
		break;
	case OF_KEY_RETURN:
		bumpDelta = 0.5;
		break;
	case 'o':
		orbit = !orbit;
		if (orbit) {
			bumpAmt = 10;
			bumpDelta = 0.0;
		} else {
			bumpAmt = 0;
		}
		break;

	case 's': // shatter
		shatter = 1;
		break;
	case 'c': // chase
		borderChase = !borderChase;
		break;
	case 'h': // hypnotize
		hypnotize = !hypnotize;
		break;
	case 'b': // border
		alphaBorderDelta = (alphaBorder == 0) ? 1 : -1;
		break;
	case 'f': // fill
		alphaFillDelta = (alphaFill == 0) ? 1 : -1;
		break;
	case 'i':
		showID = !showID;
		cout << "show ID " << showID << endl;
		break;
	case 'm':
		palMode = (palMode + 1) % NPALMODES;
		break;
	case 'j':
		jitter = 3;
		break;
	

	case '0':
		palIndex[0] = 0;
		break;
	case '1':
		palIndex[0] = 1;
		break;
	case '2':
		palIndex[0] = 2;
		break;
	case '3':
		palIndex[0] = 3;
		break;
	case '4':
		palIndex[0] = 4;
		break;
/*
	case ')':
		palIndex[19] = 0;
		break;
	case '!':
		palIndex[19] = 1;
		break;
	case '@':
		palIndex[19] = 2;
		break;
	case '#':
		palIndex[19] = 3;
		break;
	case '$':
		palIndex[19] = 4;
		break;
*/

	case OF_KEY_ESC:
		ofExit(0);
		break;

	case OF_KEY_UP:
	case OF_KEY_DOWN:
	case OF_KEY_LEFT:
	case OF_KEY_RIGHT:
		int inc = 1;
		if ((GetKeyState(VK_CONTROL) & 0x80) == 0x80) {
			inc = 10;
		}
		if ((GetKeyState(VK_SHIFT)&0x80) == 0x80) {
			switch (key) {
			case OF_KEY_UP:	vh-=inc; break;
			case OF_KEY_DOWN: vh+=inc; break;
			case OF_KEY_LEFT: vw-=inc; break;
			case OF_KEY_RIGHT: vw+=inc; break;
			}
		}
		else {
			switch (key) {
			case OF_KEY_UP:	vy-=inc; break;
			case OF_KEY_DOWN: vy+=inc; break;
			case OF_KEY_LEFT: vx-=inc; break;
			case OF_KEY_RIGHT: vx+=inc; break;
			}
		}

		cout << "viewport " << vx << "," << vy << " " << vw << "x" << vh << endl;
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
